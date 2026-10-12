/*

	EvoComb

	RTextParser.cpp

	Text File - Implementation.

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
// include files for ANSI C/C++
#include <cctype>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "tools/r_std.hpp"
#include "io/r_text_parser.hpp"
#include "tools/r_string_tools.hpp"
#include "app/r_logger.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");



//------------------------------------------------------------------------------
//
// RTextFile
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RTextParser::~RTextParser()
{
	close();
}


//------------------------------------------------------------------------------
void RTextParser::parse(const std::filesystem::path& path)
{
	close();
	if (!file_)
		file_ = std::make_unique<RIOFile>();
	file_->open(path, RIO::Read);
	path_ = path;
	fromFile_ = true;
	size_ = static_cast<size_t>(file_->size());
	begin();
}


//------------------------------------------------------------------------------
void RTextParser::parse(const char* buffer, const size_t size)
{
	close();
	path_.clear();
	buffer_ = buffer;
	size_ = buffer ? size : 0;
	fromFile_ = false;
	begin();
}


//------------------------------------------------------------------------------
char RTextParser::charAt(const size_t pos) const
{
	if (pos >= size_)
		return 0;
	if (!fromFile_)
		return buffer_[pos];
	char c = 0;
	file_->seek(static_cast<std::streamoff>(pos));
	file_->read(&c, 1, false);
	return c;
}


//------------------------------------------------------------------------------
void RTextParser::load()
{
	cur_ = charAt(pos_);
	if (cur_ == 13 && charAt(pos_ + 1) == 10)
	{
		// "\r\n" is seen as a single end of line
		pos_++;
		cur_ = 10;
	}
}


//------------------------------------------------------------------------------
void RTextParser::next()
{
	if (end())
		return;
	if (eol(cur_))
	{
		line_++;
		col_ = 1;
	}
	else
		col_++;
	pos_++;
	load();
}


//------------------------------------------------------------------------------
std::string RTextParser::readChars(size_t size)
{
	std::string res;
	while (!end() && size--)
	{
		res += cur();
		next();
	}
	return res;
}


//------------------------------------------------------------------------------
std::string RTextParser::readUntilEnd()
{
	std::string res;
	while (!end())
	{
		res += cur();
		next();
	}
	return res;
}


//------------------------------------------------------------------------------
bool RTextParser::isStringNext(const std::string& str, const bool caseSensitive, const bool skip)
{
	if (str.empty() || end() || pos_ + str.size() > size_)
		return false;

	// Compare the characters (the current one may be a "\r\n" seen as '\n')
	for (size_t i = 0; i < str.size(); ++i)
	{
		const auto c = static_cast<unsigned char>(i ? charAt(pos_ + i) : cur_);
		const auto s = static_cast<unsigned char>(str[i]);
		if (caseSensitive ? c != s : std::tolower(c) != std::tolower(s))
			return false;
	}

	// String found -> skip it
	if (skip)
		for (size_t i = 0; i < str.size(); ++i)
			next();
	return true;
}


//------------------------------------------------------------------------------
void RTextParser::skipComments()
{
	// Read Comments
	do
	{
		// Read a Comment
		while(!end() && !EndComment())
		{
			if (eol(nextChar()))
				skipEol();
			else
				next();
		}
	}
	while (beginComment());
}


//------------------------------------------------------------------------------
void RTextParser::skipSpaces()
{
	for (auto cont=true ; cont;)
	{
		// Suppose nothing was found
		cont = false;

		// Skip all spaces (by looking if the next character is a space)
		while (!end() && std::isspace(cur()))
		{
			next();
			cont = true;
		}

		// Look if after these spaces, there are not comments
		if (beginComment())
		{
			skipComments();
			cont = true;
		}
	}
}


//------------------------------------------------------------------------------
size_t RTextParser::skipCountSpaces(const char car)
{
	size_t nb(0);

	for (auto cont=true; cont;)
	{
		// Suppose nothing was found
		cont = false;

		// Skip all spaces (by looking if the next character is a space)
		while (!end() && std::isspace(cur()))
		{
			if (eol(cur()))
				nb = 0;
			else if (cur() == car)
				nb++;
			next();
			cont=true;
		}

		// Look if after these spaces, there are not comments
		if(beginComment())
		{
			nb = 0;
			skipComments();
			cont=true;
		}
	}
	return nb;
}


//------------------------------------------------------------------------------
void RTextParser::singleLineComment(const std::string& comment)
{
	comment_ = comment;
	if(comment.size() > 10)
		mThrowRIOException(this,"Maximum of 10 characters for defining comments");
}


//------------------------------------------------------------------------------
void RTextParser::multiLineComment(const std::string& begin,const std::string& end)
{
	beginComment_ = begin;
	endComment_ = end;
	if (begin.size() > 10 || end.size() > 10)
		mThrowRIOException(this,"Maximum of 10 characters for defining comments");
}


//------------------------------------------------------------------------------
std::string RTextParser::readWord()
{
	std::string res;
	bool FindComment(false);
	skipSpaces();

	while (!end() && !eol(cur()) && !std::isspace(cur()) && !((FindComment = beginComment())))
	{
		res += cur();
		next();
	}

	// Skip spaces and comments if necessary
	if (!end())
	{
		if (parseSpace_ == RParseSpaceType::SkipAllSpaces)
			skipSpaces();
		else if (FindComment)
			skipComments();
	}

	return res;
}


//------------------------------------------------------------------------------
std::string RTextParser::readToken(const std::string& endingChar)
{
	std::string res;
	bool FindComment(false);
	skipSpaces();

	while(!end() && !eol(cur()) && !std::isspace(cur()) && !(FindComment = beginComment()) && endingChar.find(cur()) == std::string::npos)
	{
		res += cur();
		next();
	}

	// Skip spaces and comments if necessary
	if(!end())
	{
		if (parseSpace_ == RParseSpaceType::SkipAllSpaces)
			skipSpaces();
		else if (FindComment)
			skipComments();
	}

	return res;
}


//------------------------------------------------------------------------------
std::string RTextParser::readTokenString(const std::string& endingStr)
{
	std::string Res;
	bool FindComment(false);
	skipSpaces();

	while((!end())&&(!eol(cur()))&&(!std::isspace(cur()))&&(!(FindComment=beginComment()))&&(!isStringNext(endingStr,true,false)))
	{
		Res+=cur();
		next();
	}

	// Skip spaces and comments if necessary
	if(!end())
	{
		if(parseSpace_== RParseSpaceType::SkipAllSpaces)
			skipSpaces();
		else if(FindComment)
			skipComments();
	}

	return Res;
}


//------------------------------------------------------------------------------
std::string RTextParser::readLine(const bool skipSpaces, const bool skipEmptyLine, const bool nextLine)
{
	if (end())
		return {};
	std::string res;

	while (!end())
	{
		while (!end() && !eol(cur()))
		{
			if (beginComment())
				skipComments();
			res += cur();
			next();
		}

		if (skipSpaces)
		{
			RStringTools::trim(res);
		}

		auto mustBreak = true;
		if (res.empty() && skipEmptyLine)
		{
			mustBreak = false;
			skipEol();
		}

		if (mustBreak)
			break;
	}

	if (!end() && nextLine)
		skipEol();

	// Return read line
	return res;
}


//------------------------------------------------------------------------------
void RTextParser::begin()
{
	line_ = 1;
	col_ = 1;
	pos_ = 0;
	activComment_ = RCommentType::NoComment;

	// Skip a byte order mark (UTF-8 or UTF-16)
	const auto c0 = static_cast<unsigned char>(charAt(0));
	const auto c1 = static_cast<unsigned char>(charAt(1));
	if (size_ >= 3 && c0 == 0xef && c1 == 0xbb && static_cast<unsigned char>(charAt(2)) == 0xbf)
		pos_ = 3;
	else if (size_ >= 2 && ((c0 == 0xfe && c1 == 0xff) || (c0 == 0xff && c1 == 0xfe)))
		pos_ = 2;

	// Get a first character
	load();

	// If necessary, skip leading comments and spaces
	if (parseSpace_ == RParseSpaceType::SkipAllSpaces)
		skipSpaces();

	start();
}


//------------------------------------------------------------------------------
void RTextParser::start()
{
	// By default nothing to do
}


//------------------------------------------------------------------------------
bool RTextParser::beginComment()
{
	auto ret=false;

	if (commentType_ == RCommentType::SingleLineComment || commentType_ == RCommentType::SingleMultiLineComment)
	{
		if (isStringNext(comment_))
		{
			activComment_ = RCommentType::SingleLineComment;
			ret = true;
		}
	}
	if (commentType_ == RCommentType::MultiLineComment || commentType_ == RCommentType::SingleMultiLineComment)
	{
		if (isStringNext(beginComment_))
		{
			activComment_ = RCommentType::MultiLineComment;
			ret = true;
		}
	}

	return ret;
}


//------------------------------------------------------------------------------
bool RTextParser::EndComment()
{
	switch (activComment_)
	{
		case RCommentType::NoComment:
			return false;
		case RCommentType::SingleLineComment:
			if (eol(cur()))
			{
				// The end of line is part of the comment
				activComment_ = RCommentType::NoComment;
				next();
				return true;
			}
			break;
		case RCommentType::MultiLineComment:
			if (isStringNext(endComment_))
			{
				activComment_ = RCommentType::NoComment;
				return true;
			}
			break;
		default:
			break;
	}

	return false;
}


//------------------------------------------------------------------------------
void RTextParser::close()
{
	if (fromFile_ && file_)
		file_->close();
	fromFile_ = false;
	buffer_ = nullptr;
	size_ = 0;
	pos_ = 0;
	cur_ = 0;
}
