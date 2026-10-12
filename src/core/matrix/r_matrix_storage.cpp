/*

	EvoComb

	RMatrixStorage.cpp

	Matrix Storage - Implementation.

	Copyright 2009-2026 by Pascal Francq (pascal@francq.info).

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2.0 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library, as a file COPYING.LIB; if not, write
	to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
	Boston, MA  02111-1307  USA

*/


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "app/r_logger.hpp"
#include "io/r_io.hpp"
#include "matrix/r_matrix_storage.hpp"
#include "matrix/r_max_matrix.hpp"
#include "matrix/r_sparse_matrix.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");



//------------------------------------------------------------------------------
//
// File format
//
//------------------------------------------------------------------------------

namespace {

//------------------------------------------------------------------------------
constexpr char cMagic[8] = {'N', 'R', 'L', 'M', 'A', 'T', 'R', 'X'};
constexpr std::uint32_t cVersion = 1;
constexpr std::uint64_t cHeaderSize = 64;
constexpr std::size_t cChunk = 4096;			// Number of values written at once


//------------------------------------------------------------------------------
// Header of the matrix file.
struct Header
{
	char magic[8];
	std::uint32_t version;
	std::uint32_t type;
	std::uint64_t nbLines;
	std::uint64_t nbCols;
	std::uint64_t stride;
	[[maybe_unused]] std::uint64_t reserved[3];
};
static_assert(sizeof(Header) == cHeaderSize, "Invalid header size");


//------------------------------------------------------------------------------
// Index of a line of a sparse matrix. The positions of the records are stored plus one, so that 0 means "no record" (a
// file extended with zeros contains empty lines).
struct LineIndex
{
	std::uint64_t first;
	std::uint64_t last;
	std::uint64_t count;
};


//------------------------------------------------------------------------------
// Record of an element of a sparse matrix (next is the position of the next record of the line plus one, 0 for the last
// one).
struct Record
{
	double value;
	std::uint64_t col;
	std::uint64_t next;
};


//------------------------------------------------------------------------------
// Value of a line of an ascending ordered sparse matrix. A line is stored as its number of values followed by nbCols
// values.
struct MaxValue
{
	std::uint64_t id;
	double value;
};


//------------------------------------------------------------------------------
void readBytes(std::fstream& file, const std::uint64_t pos, void* buffer, const std::size_t nb)
{
	file.clear();
	file.seekg(static_cast<std::streamoff>(pos));
	file.read(static_cast<char*>(buffer), static_cast<std::streamsize>(nb));
	if (!file)
		mThrowRIOException(nullptr, "Cannot read ", nb, " bytes at position ", pos);
}


//------------------------------------------------------------------------------
void writeBytes(std::fstream& file, const std::uint64_t pos, const void* buffer, const std::size_t nb)
{
	file.clear();
	file.seekp(static_cast<std::streamoff>(pos));
	file.write(static_cast<const char*>(buffer), static_cast<std::streamsize>(nb));
	if (!file)
		mThrowRIOException(nullptr, "Cannot write ", nb, " bytes at position ", pos);
}


//------------------------------------------------------------------------------
template<class T> T readAt(std::fstream& file, const std::uint64_t pos)
{
	T value{};
	readBytes(file, pos, &value, sizeof(T));
	return value;
}


//------------------------------------------------------------------------------
template<class T> void writeAt(std::fstream& file, const std::uint64_t pos, const T& value)
{
	writeBytes(file, pos, &value, sizeof(T));
}


//------------------------------------------------------------------------------
// Write a given number of times a value (in chunks).
void fillValues(std::fstream& file, std::uint64_t pos, std::uint64_t nb, const double val)
{
	const std::vector chunk((std::min<std::uint64_t>(nb, cChunk)), val);
	while (nb)
	{
		const auto n = std::min<std::uint64_t>(nb, cChunk);
		writeBytes(file, pos, chunk.data(), static_cast<std::size_t>(n) * sizeof(double));
		pos += n * sizeof(double);
		nb -= n;
	}
}


//------------------------------------------------------------------------------
std::uint64_t fileSize(std::fstream& file)
{
	file.clear();
	file.seekg(0, std::ios::end);
	return file.tellg();
}


//------------------------------------------------------------------------------
// Open a file in read/write mode (it is created if necessary).
void openFile(std::fstream& file, const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path))
		std::ofstream create(path, std::ios::binary);
	file.open(path, std::ios::in | std::ios::out | std::ios::binary);
	if (!file.is_open())
		mThrowRIOException(nullptr, "Cannot open '", path.string(), "'");
}


//------------------------------------------------------------------------------
[[noreturn]] void throwRange(const char* where, const std::size_t i, const std::size_t j, const std::size_t nbLines, const std::size_t nbCols)
{
	throw std::range_error(std::string("RMatrixStorage::") + where + " : index " + std::to_string(i) + "," + std::to_string(j) + " outside range (" + std::to_string(nbLines) + "," + std::to_string(nbCols) + ")");
}


//------------------------------------------------------------------------------
bool isSymmetric(const RMatrixType type)
{
	return type == RMatrixType::Symmetric || type == RMatrixType::SparseSymmetric;
}


//------------------------------------------------------------------------------
bool isSparse(const RMatrixType type)
{
	return type == RMatrixType::Sparse || type == RMatrixType::SparseSymmetric;
}


//------------------------------------------------------------------------------
bool isLower(const RMatrixType type)
{
	return type == RMatrixType::LowerTriangular || type == RMatrixType::Symmetric;
}

}



//------------------------------------------------------------------------------
//
// class RMatrixStorage
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RMatrixStorage::~RMatrixStorage()
{
	try
	{
		close();
	}
	catch (...)
	{
		// A destructor must not throw
	}
}


//------------------------------------------------------------------------------
void RMatrixStorage::open(const std::filesystem::path& base, const RMatrixType type)
{
	switch (type)
	{
		case RMatrixType::Normal:
		case RMatrixType::LowerTriangular:
		case RMatrixType::Symmetric:
		case RMatrixType::Sparse:
		case RMatrixType::SparseSymmetric:
		case RMatrixType::Max:
			break;
		default:
			mThrowRIOException(nullptr, "Type ", static_cast<int>(type), " is not supported");
	}
	close();
	base_ = base;
	type_ = type;
	nbLines_ = nbCols_ = stride_ = 0;

	// Matrix file
	const auto matPath = path(".mat");
	const bool exists = std::filesystem::exists(matPath) && std::filesystem::file_size(matPath) > 0;
	openFile(mat_, matPath);
	if (exists)
	{
		const auto header = readAt<Header>(mat_, 0);
		if (std::memcmp(header.magic, cMagic, sizeof(cMagic)) != 0)
		{
			mat_.close();
			mThrowRIOException(nullptr, "'", matPath.string(), "' is not a matrix storage");
		}
		if (header.version != cVersion)
		{
			mat_.close();
			mThrowRIOException(nullptr, "'", matPath.string(), "' has an unsupported version (", header.version, ")");
		}
		if (header.type != static_cast<std::uint32_t>(type))
		{
			mat_.close();
			mThrowRIOException(nullptr, "'", matPath.string(), "' contains another type of matrix");
		}
		nbLines_ = static_cast<std::size_t>(header.nbLines);
		nbCols_ = static_cast<std::size_t>(header.nbCols);
		stride_ = static_cast<std::size_t>(header.stride);
	}
	else
		writeHeader();

	// Records
	if (isSparse(type_))
		openFile(rec_, path(".rec"));
	open_ = true;
}


//------------------------------------------------------------------------------
void RMatrixStorage::close()
{
	if (!open_)
		return;
	open_ = false;
	writeHeader();
	mat_.close();
	if (rec_.is_open())
		rec_.close();
	base_.clear();
	nbLines_ = nbCols_ = stride_ = 0;
}


//------------------------------------------------------------------------------
void RMatrixStorage::checkOpen() const
{
	if (!open_)
		mThrowRIOException(nullptr, "Matrix storage not open");
}


//------------------------------------------------------------------------------
void RMatrixStorage::writeHeader()
{
	Header header{};
	std::memcpy(header.magic, cMagic, sizeof(cMagic));
	header.version = cVersion;
	header.type = static_cast<std::uint32_t>(type_);
	header.nbLines = nbLines_;
	header.nbCols = nbCols_;
	header.stride = stride_;
	writeAt(mat_, 0, header);
	mat_.flush();
}


//------------------------------------------------------------------------------
std::uint64_t RMatrixStorage::fullPos(const std::size_t i, const std::size_t j) const
{
	if (type_ == RMatrixType::Normal)
		return cHeaderSize + (static_cast<std::uint64_t>(i) * stride_ + j) * sizeof(double);
	return cHeaderSize + (static_cast<std::uint64_t>(i) * (i + 1) / 2 + j) * sizeof(double);
}


//------------------------------------------------------------------------------
std::uint64_t RMatrixStorage::maxLineSize() const
{
	return sizeof(std::uint64_t) + static_cast<std::uint64_t>(nbCols_) * sizeof(MaxValue);
}


//------------------------------------------------------------------------------
void RMatrixStorage::resizeMat(const std::uint64_t size)
{
	mat_.flush();
	std::filesystem::resize_file(path(".mat"), size);
}


//------------------------------------------------------------------------------
void RMatrixStorage::replace(std::fstream& file, const std::filesystem::path& path)
{
	file.close();
	std::filesystem::rename(std::filesystem::path(path.string() + ".new"), path);
	openFile(file, path);
}


//------------------------------------------------------------------------------
std::size_t RMatrixStorage::nbCols(const std::size_t line)
{
	checkOpen();
	if (line >= nbLines_)
		throwRange("nbCols", line, 0, nbLines_, nbCols_);
	if (type_ == RMatrixType::Max)
		return readAt<std::uint64_t>(mat_, cHeaderSize + line * maxLineSize());
	return nbCols_;
}


//------------------------------------------------------------------------------
void RMatrixStorage::clear(const double val)
{
	checkOpen();
	switch (type_)
	{
		case RMatrixType::Normal:
			fillValues(mat_, cHeaderSize, static_cast<std::uint64_t>(nbLines_) * stride_, val);
			break;
		case RMatrixType::LowerTriangular:
		case RMatrixType::Symmetric:
			fillValues(mat_, cHeaderSize, static_cast<std::uint64_t>(nbLines_) * (nbLines_ + 1) / 2, val);
			break;
		case RMatrixType::Sparse:
		case RMatrixType::SparseSymmetric:
		{
			// No records anymore and empty lines
			rec_.flush();
			std::filesystem::resize_file(path(".rec"), 0);
			resizeMat(cHeaderSize);
			resizeMat(cHeaderSize + nbLines_ * sizeof(LineIndex));
			break;
		}
		case RMatrixType::Max:
			resizeMat(cHeaderSize);
			resizeMat(cHeaderSize + nbLines_ * maxLineSize());
			break;
		default:
			break;
	}
	mat_.flush();
}


//------------------------------------------------------------------------------
void RMatrixStorage::init(const double val)
{
	checkOpen();
	if (isSparse(type_))
	{
		clear(val);
		if (val == 0.0)
			return;
		std::uint64_t recSize = 0;
		std::vector<std::pair<std::uint64_t, double>> elements;
		for (std::size_t i = 0; i < nbLines_; i++)
		{
			elements.clear();
			const auto max = type_ == RMatrixType::SparseSymmetric ? i + 1 : nbCols_;
			for (std::size_t j = 0; j < max; j++)
				elements.emplace_back(j, val);
			appendSparseLine(i, elements, recSize);
		}
		return;
	}
	if (type_ == RMatrixType::Max && val != 0.0)
		mThrowRException("init with a non-null value is not valid for an ascending ordered sparse matrix");
	clear(val);
}


//------------------------------------------------------------------------------
void RMatrixStorage::initLine(const std::size_t line, const double val)
{
	checkOpen();
	if (type_ == RMatrixType::Max)
		mThrowRException("initLine is not valid for an ascending ordered sparse matrix");
	if (line >= nbLines_)
		throwRange("initLine", line, 0, nbLines_, nbCols_);
	for (std::size_t j = 0; j < nbCols_; j++)
		if (type_ != RMatrixType::LowerTriangular || j <= line)
			write(line, j, val);
}


//------------------------------------------------------------------------------
void RMatrixStorage::initCol(const std::size_t col, const double val)
{
	checkOpen();
	if (type_ == RMatrixType::Max)
		mThrowRException("initCol is not valid for an ascending ordered sparse matrix");
	if (col >= nbCols_)
		throwRange("initCol", 0, col, nbLines_, nbCols_);
	for (std::size_t i = 0; i < nbLines_; i++)
		if (type_ != RMatrixType::LowerTriangular || col <= i)
			write(i, col, val);
}


//------------------------------------------------------------------------------
void RMatrixStorage::verifySize(const std::size_t newLines, const std::size_t newCols, const bool fill, const double val)
{
	checkOpen();
	if ((isLower(type_) || type_ == RMatrixType::SparseSymmetric) && newLines != newCols)
		throw std::range_error("RMatrixStorage::verifySize : matrix must be square");
	if (newLines == nbLines_ && newCols == nbCols_)
		return;
	switch (type_)
	{
		case RMatrixType::Normal:
			resizeNormal(newLines, newCols, fill, val);
			break;
		case RMatrixType::LowerTriangular:
		case RMatrixType::Symmetric:
			resizeLower(newLines, fill, val);
			break;
		case RMatrixType::Sparse:
		case RMatrixType::SparseSymmetric:
			resizeSparse(newLines, newCols);
			break;
		case RMatrixType::Max:
			resizeMax(newLines, newCols);
			break;
		default:
			break;
	}
	nbLines_ = newLines;
	nbCols_ = newCols;
	writeHeader();
}


//------------------------------------------------------------------------------
void RMatrixStorage::resizeNormal(const std::size_t newLines, const std::size_t newCols, const bool fill, const double val)
{
	const double def = fill ? val : 0.0;
	if (newCols > stride_)
	{
		// The capacity of columns grows: the file is rewritten
		const auto newStride = std::max(newCols, stride_ + stride_ / 2);
		{
			const auto newPath = std::filesystem::path(path(".mat").string() + ".new");
			std::fstream out(newPath, std::ios::out | std::ios::trunc | std::ios::binary);
			if (!out.is_open())
				mThrowRIOException(nullptr, "Cannot create '", newPath.string(), "'");
			constexpr Header header{};
			writeAt(out, 0, header);
			std::vector<double> line(newStride);
			const auto nbCopy = std::min(nbCols_, newCols);
			for (std::size_t i = 0; i < newLines; i++)
			{
				std::fill(line.begin(), line.end(), 0.0);
				std::size_t j = 0;
				if (i < nbLines_)
				{
					if (nbCopy)
						readBytes(mat_, fullPos(i, 0), line.data(), nbCopy * sizeof(double));
					j = nbCopy;
				}
				std::fill(line.begin() + static_cast<std::ptrdiff_t>(j), line.begin() + static_cast<std::ptrdiff_t>(newCols), def);
				writeBytes(out, cHeaderSize + static_cast<std::uint64_t>(i) * newStride * sizeof(double), line.data(), newStride * sizeof(double));
			}
		}
		replace(mat_, path(".mat"));
		stride_ = newStride;
		return;
	}

	// The lines are added or removed at the end of the file
	resizeMat(cHeaderSize + static_cast<std::uint64_t>(newLines) * stride_ * sizeof(double));

	// The new columns of the existing lines may contain old values
	if (newCols > nbCols_)
	{
		for (std::size_t i = 0, nb = std::min(nbLines_, newLines); i < nb; i++)
			fillValues(mat_, fullPos(i, nbCols_), newCols - nbCols_, def);
	}

	// The new lines contain zeros
	if (fill && newLines > nbLines_)
	{
		for (auto i = nbLines_; i < newLines; i++)
			fillValues(mat_, fullPos(i, 0), newCols, def);
	}
}


//------------------------------------------------------------------------------
void RMatrixStorage::resizeLower(const std::size_t newLines, const bool fill, const double val)
{
	const auto oldNb = static_cast<std::uint64_t>(nbLines_) * (nbLines_ + 1) / 2;
	const auto newNb = static_cast<std::uint64_t>(newLines) * (newLines + 1) / 2;
	resizeMat(cHeaderSize + newNb * sizeof(double));
	if (fill && newNb > oldNb)
		fillValues(mat_, cHeaderSize + oldNb * sizeof(double), newNb - oldNb, val);
}


//------------------------------------------------------------------------------
void RMatrixStorage::resizeSparse(const std::size_t newLines, const std::size_t newCols)
{
	// The new lines are empty (the index is extended with zeros)
	resizeMat(cHeaderSize + newLines * sizeof(LineIndex));

	// If the matrix is reduced, the records must be rewritten
	if (newLines < nbLines_ || newCols < nbCols_)
		rewriteSparse(std::min(nbLines_, newLines), newCols);
}


//------------------------------------------------------------------------------
void RMatrixStorage::resizeMax(const std::size_t newLines, const std::size_t newCols)
{
	if (newCols == nbCols_)
	{
		// The new lines are empty (the file is extended with zeros)
		resizeMat(cHeaderSize + newLines * maxLineSize());
		return;
	}

	// The lines have another size: the file is rewritten
	{
		const auto newLineSize = sizeof(std::uint64_t) + newCols * sizeof(MaxValue);
		const auto newPath = std::filesystem::path(path(".mat").string() + ".new");
		std::fstream out(newPath, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!out.is_open())
			mThrowRIOException(nullptr, "Cannot create '", newPath.string(), "'");
		constexpr Header header{};
		writeAt(out, 0, header);
		std::vector<char> line(newLineSize);
		for (std::size_t i = 0; i < newLines; i++)
		{
			std::fill(line.begin(), line.end(), 0);
			if (i < nbLines_)
			{
				// Keep the greatest values
				const auto pos = cHeaderSize + i * maxLineSize();
				const auto nb = std::min(static_cast<std::size_t>(readAt<std::uint64_t>(mat_, pos)), newCols);
				const auto count = static_cast<std::uint64_t>(nb);
				std::memcpy(line.data(), &count, sizeof(count));
				if (nb)
					readBytes(mat_, pos + sizeof(std::uint64_t), line.data() + sizeof(std::uint64_t), nb * sizeof(MaxValue));
			}
			writeBytes(out, cHeaderSize + i * newLineSize, line.data(), newLineSize);
		}
	}
	replace(mat_, path(".mat"));
}


//------------------------------------------------------------------------------
void RMatrixStorage::appendSparseLine(const std::size_t i, const std::vector<std::pair<std::uint64_t, double>>& elements, std::uint64_t& recSize)
{
	LineIndex index{0, 0, elements.size()};
	if (!elements.empty())
	{
		std::vector<Record> records;
		records.reserve(elements.size());
		for (std::size_t k = 0; k < elements.size(); k++)
		{
			const auto pos = recSize + k * sizeof(Record);
			records.push_back({elements[k].second, elements[k].first, k + 1 < elements.size() ? pos + sizeof(Record) + 1 : 0});
		}
		writeBytes(rec_, recSize, records.data(), records.size() * sizeof(Record));
		index.first = recSize + 1;
		index.last = recSize + (records.size() - 1) * sizeof(Record) + 1;
		recSize += records.size() * sizeof(Record);
	}
	writeAt(mat_, cHeaderSize + i * sizeof(LineIndex), index);
}


//------------------------------------------------------------------------------
void RMatrixStorage::rewriteSparse(const std::size_t lines, const std::size_t maxCols)
{
	const auto recPath = path(".rec");
	const auto newPath = std::filesystem::path(recPath.string() + ".new");
	std::fstream old;
	std::swap(old, rec_);
	{
		// Build the new records in rec_ (the old ones are read from old)
		rec_.open(newPath, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
		if (!rec_.is_open())
			mThrowRIOException(nullptr, "Cannot create '", newPath.string(), "'");
		std::uint64_t recSize = 0;
		std::vector<std::pair<std::uint64_t, double>> elements;
		for (std::size_t i = 0; i < lines; i++)
		{
			elements.clear();
			const auto index = readAt<LineIndex>(mat_, cHeaderSize + i * sizeof(LineIndex));
			for (auto pos = index.first; pos; )
			{
				const auto [value, col, next] = readAt<Record>(old, pos - 1);
				if (value != 0.0 && col < maxCols)
					elements.emplace_back(col, value);
				pos = next;
			}
			std::sort(elements.begin(), elements.end());
			appendSparseLine(i, elements, recSize);
		}
	}
	old.close();
	replace(rec_, recPath);
}


//------------------------------------------------------------------------------
void RMatrixStorage::compact()
{
	checkOpen();
	if (isSparse(type_))
		rewriteSparse(nbLines_, nbCols_);
}


//------------------------------------------------------------------------------
double RMatrixStorage::read(std::size_t i, std::size_t j)
{
	checkOpen();
	if (i >= nbLines_ || j >= nbCols_)
		throwRange("read", i, j, nbLines_, nbCols_);
	if (isSymmetric(type_) && j > i)
		std::swap(i, j);
	switch (type_)
	{
		case RMatrixType::Normal:
			return readAt<double>(mat_, fullPos(i, j));
		case RMatrixType::LowerTriangular:
		case RMatrixType::Symmetric:
			return j > i ? 0.0 : readAt<double>(mat_, fullPos(i, j));
		case RMatrixType::Sparse:
		case RMatrixType::SparseSymmetric:
		{
			const auto index = readAt<LineIndex>(mat_, cHeaderSize + i * sizeof(LineIndex));
			for (auto pos = index.first; pos; )
			{
				const auto [value, col, next] = readAt<Record>(rec_, pos - 1);
				if (col == j)
					return value;
				pos = next;
			}
			return 0.0;
		}
		case RMatrixType::Max:
		{
			const auto pos = cHeaderSize + i * maxLineSize();
			if (j >= readAt<std::uint64_t>(mat_, pos))
				return 0.0;
			return readAt<MaxValue>(mat_, pos + sizeof(std::uint64_t) + j * sizeof(MaxValue)).value;
		}
		default:
			return 0.0;
	}
}


//------------------------------------------------------------------------------
void RMatrixStorage::write(std::size_t i, std::size_t j, const double val)
{
	checkOpen();
	if (i >= nbLines_ || (type_ != RMatrixType::Max && j >= nbCols_))
		throwRange("write", i, j, nbLines_, nbCols_);
	if (isSymmetric(type_) && j > i)
		std::swap(i, j);
	switch (type_)
	{
		case RMatrixType::Normal:
			writeAt(mat_, fullPos(i, j), val);
			break;
		case RMatrixType::LowerTriangular:
		case RMatrixType::Symmetric:
			if (j > i)
				throw std::range_error("RMatrixStorage::write : element (" + std::to_string(i) + "," + std::to_string(j) + ") of a lower triangular matrix cannot be written");
			writeAt(mat_, fullPos(i, j), val);
			break;
		case RMatrixType::Sparse:
		case RMatrixType::SparseSymmetric:
		{
			// Look if the element exists
			const auto indexPos = cHeaderSize + i * sizeof(LineIndex);
			auto index = readAt<LineIndex>(mat_, indexPos);
			for (auto pos = index.first; pos; )
			{
				const auto [value, col, next] = readAt<Record>(rec_, pos - 1);
				if (col == j)
				{
					if (value != val)
						writeAt(rec_, pos - 1, val);		// The value is the first field
					return;
				}
				pos = next;
			}
			if (val == 0.0)
				return;

			// Add a record at the end of the line
			const auto recPos = fileSize(rec_);
			writeAt(rec_, recPos, Record{val, j, 0});
			if (index.last)
				writeAt(rec_, index.last - 1 + offsetof(Record, next), recPos + 1);
			else
				index.first = recPos + 1;
			index.last = recPos + 1;
			index.count++;
			writeAt(mat_, indexPos, index);
			break;
		}
		case RMatrixType::Max:
		{
			// Read the line
			const auto pos = cHeaderSize + i * maxLineSize();
			std::vector<MaxValue> values((readAt<std::uint64_t>(mat_, pos)));
			if (!values.empty())
				readBytes(mat_, pos + sizeof(std::uint64_t), values.data(), values.size() * sizeof(MaxValue));

			// Remove the old value of the identifier
			if (const auto old = std::find_if(values.begin(), values.end(), [j](const MaxValue& value) { return value.id == j; }); old != values.end())
				values.erase(old);
			else if (values.size() >= nbCols_)
			{
				// The line is full: the value replaces the smallest one if it is greater
				if (nbCols_ == 0 || val <= values.back().value)
					return;
				values.pop_back();
			}

			// Insert the value after the values that are greater or equal
			const auto where = std::find_if(values.begin(), values.end(), [val](const MaxValue& value) { return value.value < val; });
			values.insert(where, MaxValue{j, val});
			writeAt(mat_, pos, static_cast<std::uint64_t>(values.size()));
			writeBytes(mat_, pos + sizeof(std::uint64_t), values.data(), values.size() * sizeof(MaxValue));
			break;
		}
		default:
			break;
	}
}


//------------------------------------------------------------------------------
void RMatrixStorage::load(RGenericMatrix& matrix)
{
	checkOpen();
	if (matrix.type() != type_)
		mThrowRIOException(nullptr, "Cannot load '", base_.string(), "': invalid type of matrix");
	switch (type_)
	{
		case RMatrixType::Normal:
		{
			matrix.verifySize(nbLines_, nbCols_);
			std::vector<double> line(nbCols_);
			for (std::size_t i = 0; i < nbLines_; i++)
			{
				if (nbCols_)
					readBytes(mat_, fullPos(i, 0), line.data(), nbCols_ * sizeof(double));
				for (std::size_t j = 0; j < nbCols_; j++)
					matrix(i, j) = line[j];
			}
			break;
		}
		case RMatrixType::LowerTriangular:
		case RMatrixType::Symmetric:
		{
			matrix.verifySize(nbLines_, nbCols_);
			std::vector<double> line(nbLines_);
			for (std::size_t i = 0; i < nbLines_; i++)
			{
				readBytes(mat_, fullPos(i, 0), line.data(), (i + 1) * sizeof(double));
				for (std::size_t j = 0; j <= i; j++)
					matrix(i, j) = line[j];
			}
			break;
		}
		case RMatrixType::Sparse:
		case RMatrixType::SparseSymmetric:
		{
			auto* sparse = dynamic_cast<RSparseMatrix*>(&matrix);
			if (!sparse)
				mThrowRIOException(nullptr, "Cannot load '", base_.string(), "': the matrix is not a RSparseMatrix");
			sparse->verifySize(nbLines_, nbCols_);
			sparse->clear();
			for (std::size_t i = 0; i < nbLines_; i++)
			{
				auto& values = (*sparse)[i].values_;
				const auto index = readAt<LineIndex>(mat_, cHeaderSize + i * sizeof(LineIndex));
				for (auto pos = index.first; pos; )
				{
					const auto [value, col, next] = readAt<Record>(rec_, pos - 1);
					if (value != 0.0)
						values.push_back({static_cast<std::size_t>(col), value});
					pos = next;
				}
				std::sort(values.begin(), values.end(), [](const RSparseValue& a, const RSparseValue& b) {
					return a.id_ < b.id_;
				});
			}
			break;
		}
		case RMatrixType::Max:
		{
			auto* max = dynamic_cast<RMaxMatrix*>(&matrix);
			if (!max)
				mThrowRIOException(nullptr, "Cannot load '", base_.string(), "': the matrix is not a RMaxMatrix");
			max->verifySize(nbLines_, nbCols_);
			max->clear();
			std::vector<MaxValue> values;
			for (std::size_t i = 0; i < nbLines_; i++)
			{
				const auto pos = cHeaderSize + i * maxLineSize();
				values.resize(readAt<std::uint64_t>(mat_, pos));
				if (!values.empty())
					readBytes(mat_, pos + sizeof(std::uint64_t), values.data(), values.size() * sizeof(MaxValue));
				auto& line = (*max)[i].values_;
				for (const auto&[id, value] : values)
					line.push_back({static_cast<std::size_t>(id), value});
			}
			break;
		}
		default:
			break;
	}
}


//------------------------------------------------------------------------------
void RMatrixStorage::save(const RGenericMatrix& matrix)
{
	checkOpen();
	if (matrix.type() != type_)
		mThrowRIOException(nullptr, "Cannot save '", base_.string(), "': invalid type of matrix");
	switch (type_)
	{
		case RMatrixType::Normal:
		{
			verifySize(matrix.nbLines(), matrix.nbCols());
			std::vector<double> line(nbCols_);
			for (std::size_t i = 0; i < nbLines_; i++)
			{
				for (std::size_t j = 0; j < nbCols_; j++)
					line[j] = matrix(i, j);
				if (nbCols_)
					writeBytes(mat_, fullPos(i, 0), line.data(), nbCols_ * sizeof(double));
			}
			break;
		}
		case RMatrixType::LowerTriangular:
		case RMatrixType::Symmetric:
		{
			verifySize(matrix.nbLines(), matrix.nbCols());
			std::vector<double> line(nbLines_);
			for (std::size_t i = 0; i < nbLines_; i++)
			{
				for (std::size_t j = 0; j <= i; j++)
					line[j] = matrix(i, j);
				writeBytes(mat_, fullPos(i, 0), line.data(), (i + 1) * sizeof(double));
			}
			break;
		}
		case RMatrixType::Sparse:
		case RMatrixType::SparseSymmetric:
		{
			const auto* sparse = dynamic_cast<const RSparseMatrix*>(&matrix);
			if (!sparse)
				mThrowRIOException(nullptr, "Cannot save '", base_.string(), "': the matrix is not a RSparseMatrix");
			verifySize(matrix.nbLines(), matrix.nbCols());
			clear();
			std::uint64_t recSize = 0;
			std::vector<std::pair<std::uint64_t, double>> elements;
			for (std::size_t i = 0; i < nbLines_; i++)
			{
				elements.clear();
				for (const auto& [j, value] : sparse->lines()[i])
					if (value != 0.0)
						elements.emplace_back(j, value);
				appendSparseLine(i, elements, recSize);
			}
			break;
		}
		case RMatrixType::Max:
		{
			const auto* max = dynamic_cast<const RMaxMatrix*>(&matrix);
			if (!max)
				mThrowRIOException(nullptr, "Cannot save '", base_.string(), "': the matrix is not a RMaxMatrix");
			auto cols = max->nbCols();
			if (!cols)
				for (const auto& line : max->lines())
					cols = std::max(cols, line.size());
			verifySize(max->nbLines(), cols);
			const auto lineSize = maxLineSize();
			std::vector<MaxValue> values;
			for (std::size_t i = 0; i < nbLines_; i++)
			{
				values.clear();
				for (const auto& [id, value] : max->lines()[i])
					if (values.size() < nbCols_)
						values.push_back({id, value});
				const auto pos = cHeaderSize + i * lineSize;
				writeAt(mat_, pos, static_cast<std::uint64_t>(values.size()));
				if (!values.empty())
					writeBytes(mat_, pos + sizeof(std::uint64_t), values.data(), values.size() * sizeof(MaxValue));
			}
			break;
		}
		default:
			break;
	}
	mat_.flush();
	if (rec_.is_open())
		rec_.flush();
}


//------------------------------------------------------------------------------
std::size_t RMatrixStorage::infoSize() const
{
	checkOpen();
	const auto infoPath = path(".info");
	return std::filesystem::exists(infoPath) ? std::filesystem::file_size(infoPath) : 0;
}


//------------------------------------------------------------------------------
void RMatrixStorage::readInfo(const std::size_t pos, void* buffer, const std::size_t nb) const
{
	if (pos + nb > infoSize())
		mThrowRIOException(nullptr, "No information of ", nb, " bytes at position ", pos, " in '", base_.string(), "'");
	std::fstream info;
	openFile(info, path(".info"));
	readBytes(info, pos, buffer, nb);
}


//------------------------------------------------------------------------------
void RMatrixStorage::writeInfo(const std::size_t pos, const void* buffer, const std::size_t nb) const
{
	checkOpen();
	std::fstream info;
	openFile(info, path(".info"));
	if (const auto size = fileSize(info); pos > size)
	{
		// Fill the gap with zeros
		const std::vector<char> zeros(pos - size, 0);
		writeBytes(info, size, zeros.data(), zeros.size());
	}
	writeBytes(info, pos, buffer, nb);
}
