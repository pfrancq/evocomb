/*

	EvoComb

	RTextParser.hpp

	Text File - Header.

	Copyright 1999-2026 by Pascal Francq (pascal@francq.info).
	Copyright 1999-2008 by the Université Libre de Bruxelles (ULB).

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
#include <charconv>
#include <filesystem>
#include <memory>
#include <string>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/r_io_file.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
* The RCommentType enum represents the different style of comments used which text files. The comments are skipped when
* the file is read.
*/
enum class RCommentType
{
	NoComment                /** No comments are possible. */,
	SingleLineComment        /** Only single line comments are possible. */,
	MultiLineComment         /** Only multiple line comments are possible. */,
	SingleMultiLineComment   /** Both type of comments are possible. */
};


	//------------------------------------------------------------------------------
/**
 * The ParseSpaceType enum represents the different way to handle spaces. If SkipAllSpaces is chosen, leading spaces and
 * empty lines are not seen by the programmer.
 */
enum class RParseSpaceType
{
	LeaveSpaces              /** Leave spaces. */,
	SkipAllSpaces            /** Skip all spaces. */
};


//------------------------------------------------------------------------------
/**
* The RTextFile class implements some basic functions needed when parsing text files.
*
* The file can contain some comments. These comments can be either on a single line, beginning with a given set of
* characters ('%', by default) and ending at the end of the line, or be multi-line, beginning and ending with
* corresponding set of characters (C comments delimiters by default).
* @short Text File.
*/
class RTextParser
{
public:

	/**
	 * Default constructor.
	 */
	RTextParser() = default;

	/**
	* Destruct the file.
	*/
	virtual ~RTextParser();

	/**
	 * Open the file
	 * @param path            Path of the file.
	 */
	void parse(const std::filesystem::path& path);

	/**
	 * Return the current path of the text parser (can be empty if a buffer is passed).
	 * @return a path.
	 */
	[[nodiscard]] std::filesystem::path path() const { return path_; }

	/**
	 * Parse a text in memory (the text is not copied: it must exist until the end of the parsing).
	 * @param buffer          Text.
	 * @param size            Size of the text.
	 */
	void parse(const char* buffer, size_t size);

	/**
	* Return true if all the characters were read (there is no current character anymore).
	*/
	[[nodiscard]] bool end() const { return pos_ >= size_; }

	/**
	* Get the current character (the next one to treat), or 0 if the end is reached. It is the same as cur.
	*/
	[[nodiscard]] char nextChar() const { return cur_; }

	/**
	 * Get the current character (the next one to treat), or 0 if the end is reached. A "\r\n" end of line is seen as a
	 * single '\n' character.
	 */
	[[nodiscard]] char cur() const { return cur_; }

	/**
	* Get the next characters.
	* @param size            Number to characters to read.
	* @return a std::string.
	*/
	[[nodiscard]] std::string readChars(size_t size);

	/**
	* Read the rest of the file.
	* @return A std::string containing the file.
	*/
	[[nodiscard]] std::string readUntilEnd();

	/**
	* Read to next character.
	*/
	void next();

	/**
	* Return true if the character defines the end of a line.
	* @param car            Character.
	*/
	static bool eol(const char car) { return car==10 || car==13; }

	/**
	* Verify if a string is the next one in the buffer. If the string is found, it may be skipped.
	* @param str             The string to find
	* @param caseSensitive   Is the search case sensitive.
	* @param skip            Must the string be skipped?
	* @return True if the string is found.
	*/
	bool isStringNext(const std::string& str, bool caseSensitive=true, bool skip=true);

	/**
	* This function skip comments (Only used if read mode).
	*/
	void skipComments();

	/**
	* This function skip spaces.
	*/
	void skipSpaces();

	/**
	 * This function skip spaces and new lines.
	 */
	void skipSpacesEol()
	{
		while (true)
		{
			if (std::isspace(cur()))
				next();
			else if (eol(cur()))
				skipEol();
			else
				break;
		}
	}

	/**
	* This function skip spaces and counts the number of a given character at the beginning on the last line read (Only
	* used if read mode).
	* @param car             Character (must be a space character such as a tab).
	*/
	size_t skipCountSpaces(char car);

	/**
	* Set the style of comments.
	* @param style          The Style.
	*/
	void commentType(const RCommentType style) { commentType_ = style; }

	/**
	 * Set the way spaces are treated.
	 */
	void parseSpace(const RParseSpaceType parse) { parseSpace_ = parse; }

	/**
	 * Set the way spaces are treated.
	 */
	[[nodiscard]] RParseSpaceType parseSpace() const { return parseSpace_; }

	/**
	* Set the string of comments when single line.
	* @param comment			This string represent the begin of a comment.
	*/
	void singleLineComment(const std::string& comment);

	/**
	* Set the begin and the end strings of comments when in multi-line.
	* @param begin				This string represent the begin of a comment.
	* @param end				This string represent the end of a comment.
	*/
	void multiLineComment(const std::string& begin,const std::string& end) ;

	/**
	* Get the next word contained in the file. A word is a suite of characters delimited by spaces.
	*/
	std::string readWord();

	/**
	* Get the next token contained in the file. A token is a suite of characters delimited either by spaces or by a
	* specific ending character. The ending character is not skipped.
	* @param endingChar      String containing all possible ending characters.
	*/
	std::string readToken(const std::string& endingChar);

	/**
	* Get the next token contained in the file, i.e a suite of characters delimited either by spaces or by a specific
	* string. The ending string is not skipped.
	* @param endingStr       Ending String.
	*/
	std::string readTokenString(const std::string& endingStr);

	/**
	* Return the next entire line in the file. Depending on the `nextLine` parameter, the skips or not the end of line.
	* @param skipSpaces				Skip spaces starting or ending the line (true by default)?
	* @param skipEmptyLine			Skip empty lines (true by default)?
	* @param nextLine				S
	*/
	std::string readLine(bool skipSpaces = true, bool skipEmptyLine = true, bool nextLine = true);

	/**
	 * Read a number.
	 * @tparam I			Type of the number.
	 * @return the number.
	 */
	template<typename I>
		I readNumber()
	{
		skipSpaces();
		auto str = readWord();
		I value;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
		if (ec != std::errc())
			// LOG_MODULE cannot be used in a header: the module is given explicitly.
			throw RIOException(this, "r.core", __FILE__, __LINE__, "Cannot read number");
		return value;
	}

	/**
	* Return the actual line number.
	*/
	[[nodiscard]] unsigned long line() const { return line_; }

	/**
	* Return the actual line number.
	*/
	[[nodiscard]] unsigned long col() const { return col_; }

private:

	/**
	* Go to the beginning of the file.
	*/
	void begin();

	/**
	 * Start the real parsing.
	 */
	virtual void start();

	/**
	* This function returns true if the current position is the beginning of a comment (Only used if in read mode).
	*/
	bool beginComment();

	/**
	* This function returns true if the current position is the ending of a comment and skip the end characters if
	* MultiLineComment.
	*/
	bool EndComment();

	/**
	* Close the file.
	*/
	void close();

	/**
	 * Get a character of the text (0 if the position is beyond the end).
	 * @param pos            Position.
	 */
	[[nodiscard]] char charAt(size_t pos) const;

	/**
	 * Set the current character from the current position (a "\r\n" end of line is seen as a single '\n').
	 */
	void load();

	/**
	* This function skip a end of line.
	*/
	void skipEol()
	{
		if (cur_ == 10 || cur_ == 13)
			next();
	}

	//-----------------------------------------------------
	std::filesystem::path path_;									///< Path of the file to read (if any)
	std::unique_ptr<RIOFile> file_;									///< File to read (if necessary).
	const char* buffer_ = nullptr;									///< Text to parse (if no file).
	std::string comment_ = "%";										///< String representing a single line comment.
	std::string beginComment_ = "/*";								///< String representing the beginning of a multi-line comment.
	std::string endComment_ = "*/";									///< String representing the ending of a multi-line comment.
	RCommentType commentType_ = RCommentType::NoComment;			///< The type of comments that are using for this file.
	RCommentType activComment_ = RCommentType::NoComment;			///< The type of comments currently active.
	RParseSpaceType parseSpace_ = RParseSpaceType::SkipAllSpaces;	///< How to handle spaces.
	size_t line_ = 1;												///< Line of the current character.
	size_t col_ = 1;												///< Column of the current character.
	char cur_ = 0;													///< Current character (0 at the end).
	size_t size_ = 0;												///< Size of the text.
	size_t pos_ = 0;												///< Position of the current character in the text.
	bool fromFile_ = false;											///< Define if a file is parsed or an external buffer
};


}  //-------- End of namespace evocomb ---------------------------------------------
