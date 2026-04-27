/*

	EvoComb

	RMatrixStorage.hpp

	Matrix Storage - Header.

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
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <type_traits>
#include <utility>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "matrix/r_generic_matrix.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RMatrixStorage class stores a matrix on disk and gives access to its elements without loading it in memory. It is
 * useful for matrices that are too large for the memory, or to keep a matrix (for example the similarities between
 * objects) between two executions of a program.
 *
 * The storage is identified by a base path and uses up to three files:
 * - "<base>.mat" contains a header (type and size of the matrix) followed by
 *   the elements (full matrices), by the index of the lines (sparse matrices)
 *   or by the lines (ascending ordered sparse matrices).
 * - "<base>.rec" contains the elements of a sparse matrix: each line is a
 *   linked list of records (value, column, next record).
 * - "<base>.info" contains free information that the developer can add (see
 *   readInfo and writeInfo). It is not modified when the size of the matrix
 *   changes.
 *
 * The supported types are:
 * - RMatrixType::Normal: the lines are stored one after the other with a
 *   capacity of columns that grows by at least 50% when the number of
 *   columns increases, so that adding lines or columns rarely needs to
 *   rewrite the file.
 * - RMatrixType::LowerTriangular and RMatrixType::Symmetric: square matrices
 *   whose lower part is stored line by line (adding lines only appends
 *   elements).
 * - RMatrixType::Sparse and RMatrixType::SparseSymmetric: only the non-null
 *   elements are stored (the lower part for a symmetric matrix). Writing a
 *   null value in an existing element keeps its record: use compact to
 *   remove them.
 * - RMatrixType::Max: each line contains at most nbCols() values ordered by
 *   decreasing values, each one associated to an identifier (see
 *   RMaxMatrix). read(i,j) returns the j-th greatest value of line i, and
 *   write(i,id,value) sets the value associated to identifier id.
 *
 * The values are stored in the native binary format of the machine.
 * @code
 * evocomb::RMatrixStorage storage;
 * storage.open("/tmp/sims", evocomb::RMatrixType::Symmetric);
 * storage.verifySize(1000, 1000, true, 0.0);
 * storage.write(10, 5, 0.8);
 * std::cout << storage.read(5, 10) << std::endl;     // 0.8
 * storage.writeInfo(0, std::size_t{42});              // Information of the developer.
 * storage.close();
 * @endcode
 * @short Matrix Storage.
 */
class RMatrixStorage
{
public:

	/**
	 * Construct the storage.
	 */
	RMatrixStorage() = default;

	/**
	 * Destruct the storage (it is closed if necessary).
	 */
	~RMatrixStorage();

	/**
	 * Open the storage. If it does not exist, an empty matrix is created.
	 * @param base				Base path of the files.
	 * @param type				Type of the matrix.
	 * @exception RIOException	The type is not supported, the files cannot
	 *							be opened, or the files contain another type
	 *							of matrix.
	 */
	void open(const std::filesystem::path& base, RMatrixType type);

	/**
	 * Close the storage.
	 */
	void close();

	/**
	 * Look if the storage is open.
	 */
	[[nodiscard]] bool isOpen() const { return open_; }

	/**
	 * Get the base path of the files.
	 */
	[[nodiscard]] const std::filesystem::path& base() const { return base_; }

	/**
	 * Get the type of the matrix.
	 */
	[[nodiscard]] RMatrixType type() const { return type_; }

	/**
	 * Get the number of lines of the matrix.
	 */
	[[nodiscard]] std::size_t nbLines() const { return nbLines_; }

	/**
	 * Get the number of columns of the matrix (the maximal number of values of each line for an ascending ordered
	 * sparse matrix).
	 */
	[[nodiscard]] std::size_t nbCols() const { return nbCols_; }

	/**
	 * Get the number of values of a line. It is the number of columns except for an ascending ordered sparse matrix
	 * where it is the number of values stored in the line.
	 * @param line				Line.
	 * @exception std::range_error The line is outside the matrix.
	 */
	[[nodiscard]] std::size_t nbCols(std::size_t line);

	/**
	 * Clear the matrix: the elements of a full matrix are set to a given value, and a sparse matrix has no values
	 * anymore.
	 * @param val				Value.
	 */
	void clear(double val = NAN);

	/**
	 * Set all the elements to a given value. For a sparse matrix, a non-null value creates all the elements.
	 * @param val				Value.
	 * @exception RException	The value is not null and the matrix is an
	 *							ascending ordered sparse matrix.
	 */
	void init(double val);

	/**
	 * Set all the elements of a line to a given value.
	 * @param line				Line.
	 * @param val				Value.
	 * @exception RException	The matrix is an ascending ordered sparse
	 *							matrix.
	 */
	void initLine(std::size_t line, double val);

	/**
	 * Set all the elements of a column to a given value.
	 * @param col				Column.
	 * @param val				Value.
	 * @exception RException	The matrix is an ascending ordered sparse
	 *							matrix.
	 */
	void initCol(std::size_t col, double val);

	/**
	 * Change the size of the matrix. The elements outside the new size are removed. The new elements of a full matrix
	 * are set to val if fill is true (0 otherwise), while a sparse matrix has no values for them. For an ascending
	 * ordered sparse matrix, reducing the number of columns removes the smallest values of each line.
	 * @param newLines			New number of lines.
	 * @param newCols			New number of columns.
	 * @param fill				Fill the new elements with val.
	 * @param val				Value of the new elements.
	 * @exception std::range_error A triangular or symmetric matrix is not
	 *							square.
	 */
	void verifySize(std::size_t newLines, std::size_t newCols, bool fill = false, double val = NAN);

	/**
	 * Load the matrix into a matrix of the same type (resized if necessary). A sparse matrix is emptied first.
	 * @param matrix			Matrix.
	 * @exception RIOException	The types are different.
	 */
	void load(RGenericMatrix& matrix);

	/**
	 * Save a matrix of the same type (the storage is resized if necessary). For an RMaxMatrix without a maximal number
	 * of values, the number of columns of the storage is the size of its largest line.
	 * @param matrix			Matrix.
	 * @exception RIOException	The types are different.
	 */
	void save(const RGenericMatrix& matrix);

	/**
	 * Read an element.
	 * @param i					Line.
	 * @param j					Column (the rank of the value for an ascending
	 *							ordered sparse matrix).
	 * @return the value (0 for a missing element of a sparse matrix).
	 * @exception std::range_error The indexes are outside the matrix.
	 */
	[[nodiscard]] double read(std::size_t i, std::size_t j);

	/**
	 * Write an element.
	 * @param i					Line.
	 * @param j					Column (the identifier associated to the value
	 *							for an ascending ordered sparse matrix: if the
	 *							line is full, the value is only inserted if it
	 *							is greater than the smallest one, which is then
	 *							removed).
	 * @param val				Value.
	 * @exception std::range_error The indexes are outside the matrix, or an
	 *							element above the diagonal of a lower triangular
	 *							matrix is written.
	 */
	void write(std::size_t i, std::size_t j, double val);

	/**
	 * Rewrite the records of a sparse matrix: the null values are removed and the elements of each line become
	 * contiguous and ordered by column. It does nothing for the other types.
	 */
	void compact();

	/**
	 * Get the size of the information of the developer (in bytes).
	 */
	[[nodiscard]] std::size_t infoSize() const;

	/**
	 * Look if the storage contains information of the developer.
	 */
	[[nodiscard]] bool hasInfo() { return infoSize() > 0; }

	/**
	 * Read information of the developer.
	 * @param pos				Position of the information (in bytes).
	 * @param buffer			Buffer receiving the information.
	 * @param nb				Number of bytes to read.
	 * @exception RIOException	The information does not exist.
	 */
	void readInfo(std::size_t pos, void* buffer, std::size_t nb) const;

	/**
	 * Write information of the developer.
	 * @param pos				Position of the information (in bytes).
	 * @param buffer			Buffer containing the information.
	 * @param nb				Number of bytes to write.
	 */
	void writeInfo(std::size_t pos, const void* buffer, std::size_t nb) const;

	/**
	 * Read information of the developer.
	 * @tparam T				Type of the information (trivially copyable).
	 * @param pos				Position of the information (in bytes).
	 * @exception RIOException	The information does not exist.
	 */
	template<class T> [[nodiscard]] T readInfo(const std::size_t pos)
	{
		static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
		T value{};
		readInfo(pos, &value, sizeof(T));
		return value;
	}

	/**
	 * Write information of the developer.
	 * @tparam T				Type of the information (trivially copyable).
	 * @param pos				Position of the information (in bytes).
	 * @param value				Information.
	 */
	template<class T> void writeInfo(const std::size_t pos, const T& value)
	{
		static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
		writeInfo(pos, &value, sizeof(T));
	}

	//-----------------------------------------------------
	RMatrixStorage(const RMatrixStorage&) = delete;
	RMatrixStorage& operator=(const RMatrixStorage&) = delete;

private:

	/**
	 * Verify that the storage is open.
	 */
	void checkOpen() const;

	/**
	 * Write the header of the matrix file.
	 */
	void writeHeader();

	/**
	 * Get the position of an element of a full matrix in the matrix file.
	 * @param i					Line.
	 * @param j					Column (j ≤ i for a triangular matrix).
	 */
	[[nodiscard]] std::uint64_t fullPos(std::size_t i, std::size_t j) const;

	/**
	 * Get the size of a line of an ascending ordered sparse matrix.
	 */
	[[nodiscard]] std::uint64_t maxLineSize() const;

	/**
	 * Resize the matrix file.
	 * @param size				New size (in bytes).
	 */
	void resizeMat(std::uint64_t size);

	/**
	 * Replace a file by a new one (built at the path of the file followed by ".new") and reopen it.
	 * @param file				File.
	 * @param path				Path of the file.
	 */
	static void replace(std::fstream& file, const std::filesystem::path& path);

	/**
	 * Change the size of a normal matrix.
	 */
	void resizeNormal(std::size_t newLines, std::size_t newCols, bool fill, double val);

	/**
	 * Change the size of a lower triangular or symmetric matrix.
	 */
	void resizeLower(std::size_t newLines, bool fill, double val);

	/**
	 * Change the size of a sparse matrix.
	 */
	void resizeSparse(std::size_t newLines, std::size_t newCols);

	/**
	 * Change the size of an ascending ordered sparse matrix.
	 */
	void resizeMax(std::size_t newLines, std::size_t newCols);

	/**
	 * Rewrite the records of a sparse matrix (see compact).
	 * @param lines				Number of lines to keep.
	 * @param maxCols			Only the columns smaller than maxCols are kept.
	 */
	void rewriteSparse(std::size_t lines, std::size_t maxCols);

	/**
	 * Append the elements of an empty line of a sparse matrix at the end of the records.
	 * @param i					Line.
	 * @param elements			Elements (column, value) ordered by column.
	 * @param recSize			Size of the records file (updated).
	 */
	void appendSparseLine(std::size_t i, const std::vector<std::pair<std::uint64_t, double>>& elements, std::uint64_t& recSize);

	/**
	 * Get the path of a file of the storage.
	 * @param ext				Extension (for example ".mat").
	 */
	[[nodiscard]] std::filesystem::path path(const char* ext) const { return {base_.string() + ext}; }

	//-----------------------------------------------------
	std::filesystem::path base_;				///< Base path.
	std::fstream mat_;							///< Matrix file.
	std::fstream rec_;							///< Records (sparse matrices).
	RMatrixType type_ = RMatrixType::Normal;	///< Type of the matrix.
	std::size_t nbLines_ = 0;					///< Number of lines.
	std::size_t nbCols_ = 0;					///< Number of columns.
	std::size_t stride_ = 0;					///< Capacity of columns (normal matrices).
	bool open_ = false;							///< Storage is open?
};


}  //-------- End of namespace evocomb ---------------------------------------------
