/*

	EvoComb

	RIO.hpp

	Generic Class - Header.

	Copyright 2002-2026 by Pascal Francq (pascal@francq.info).
	Copyright 2002-2008 by the Université Libre de Bruxelles (ULB).

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
#include <filesystem>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "tools/r_exception.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Forward declaration
class RIOFile;
class RTextParser;


//------------------------------------------------------------------------------
/**
* The RIO class provides some I/O types.
* @short I/O Types.
*/
class RIO
{
public:

	/**
	* The ModeType enum represents the different mode that can be used to open a file.
	* @short File Access Mode
	*/
	enum ModeType
	{
		Append                       /** Append to an existing file. If the file does not exist, it is created.*/,
		Read                         /** Open a file to read it.*/,
		Write                        /** Open a file to write it. If the file doesn't exist, it is created.*/,
		ReadWrite                    /** Open a file to read and write it. If the file doesn't exist, it is created.*/,
		Create                       /** Create a new file. If the file already exists, it is destruct.*/,
		Undefined                    /** The mode of the file is undefined.*/
	};
};


//------------------------------------------------------------------------------
/**
 * This class provides a "smart" temporary file. In practice, once the name of a temporary file is asked, when the
 * instance is deleted, the temporary file is deleted. Of course, the temporary file may be explicitly leave if
 * necessary.
 *
 * It can be useful in an exception environment, since temporary files are always removed.
 * @code
 * void BadFunction(void)
 * {
 * 	RSmartTempFile tmp;
 * 	RTextFile Text(tmp.path());
 * 	Text.Open(RIO::Create);
 * 	int Wrong (2/0); // This will generate an exception -> Temporary file is removed
 * }
 * @endcode
 * @short Smart Temporary File.
 */
class RSmartTempFile
{
public:

	/**
	 * Constructor.
	 * @param remove     If `true`, the file must be removed.
	 */
	explicit RSmartTempFile(bool remove = true);

	/**
	 * Get the name of the temporary file.
	 */
	std::filesystem::path path() { return path_; }

	/**
	 * Should the file be removed?
	 * @param remove     If `true`, the file must be removed.
	 */
	void remove(bool remove);

	/**
	 * Destruct. If the name was requested, the temporary file is removed.
	 */
	~RSmartTempFile();

private:

    //-----------------------------------------------------
	std::filesystem::path path_;					///< Name of the temporary file.
	bool remove_;									///< Temporary file must be removed.
};


//------------------------------------------------------------------------------
/**
* The RIOException class provides a basic representation for an IO exception.
* @short IO Exception.
*/
class RIOException : public RException
{
public:

	/**
	* Construct an IO exception.
	* @param str                      Message of the error.
	*/
	explicit RIOException(const std::string& str);

	/**
	* Construct an IO exception with the same information as the cLog* macros. The message is preceded by the path and
	* the position in the file.
	* @see The mThrowRIOException macro.
	* @param file                     File process when the error occurs.
	* @param module                   Name of the module.
	* @param srcFile                  Name of the source file.
	* @param line                     Line in the source file.
	* @param str                      Message of the error.
	*/
	RIOException(const RIOFile* file, std::string_view module, std::string_view srcFile, int line, const std::string& str);

	/**
	* Construct an IO exception with the same information as the cLog* macros. The message is preceded by the path and
	* the position (line:col) in the file.
	* @see The mThrowRIOException macro.
	* @param file                     File process when the error occurs.
	* @param module                   Name of the module.
	* @param srcFile                  Name of the source file.
	* @param line                     Line in the source file.
	* @param str                      Message of the error.
	*/
	RIOException(const RTextParser* file, std::string_view module, std::string_view srcFile, int line, const std::string& str);

	/**
	* Construct an IO exception not related to a particular file object (for example, mThrowRIOException(nullptr,
	* "Cannot remove ", path)).
	* @param module                   Name of the module.
	* @param srcFile                  Name of the source file.
	* @param line                     Line in the source file.
	* @param str                      Message of the error.
	*/
	RIOException(std::nullptr_t, std::string_view module, std::string_view srcFile, int line, const std::string& str);

private:

	/**
	* Build the message with the information on the file.
	*/
	static std::string detail(const RIOFile* file, const std::string& str);

	/**
	* Build the message with the information on the file.
	*/
	static std::string detail(const RTextParser* file, const std::string& str);
};


}  //-------- End of namespace evocomb -----------------------------------------------


//------------------------------------------------------------------------------
// Macro to generate a RIOException. As for the cLog* macros, the source file must declare its module with cLogModule.
// The first parameter is the file object (RIOFile* or RTextParser*) or nullptr, followed by the same values as the
// cLog* macros:
//   mThrowRIOException(this, "Cannot read ", size, " bytes");
#define mThrowRIOException(file, ...) throw evocomb::RIOException(file, evocomb_log_module_name, __FILE__, __LINE__, evocomb::RConcat::concat(__VA_ARGS__))
