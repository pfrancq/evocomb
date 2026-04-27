/*

	EvoComb

	RException.hpp

	Basic Exception - Header.

	Copyright 2007-2026 by Pascal Francq (pascal@francq.info).
	Copyright 2007-2008 by the Université Libre de Bruxelles (ULB).

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/



//------------------------------------------------------------------------------
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <string>
#include <string_view>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "tools/r_concat.hpp"


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
* The RException class provides a basic representation for an exception.
* @short Basic Exception.
*/
class RException : public std::exception
{
public:

	/**
	* Construct an exception.
	*/
	RException() = default;

	/**
	* Construct an exception.
	* @param msg                      Message of the error.
	*/
	explicit RException(std::string msg);

	/**
	* Construct an exception with the same information as the LOG_* macros. Only the file name of file is kept.
	* @see The mThrowRException macro.
	* @param module                   Name of the module.
	* @param file                     Name of the source file.
	* @param line                     Line in the source file.
	* @param msg                      Message of the error.
	*/
	RException(std::string_view module, std::string_view file, int line, std::string msg);

	/**
	* Destructor.
	*/
	~RException() override = default;

	/**
	* Get the message of the exception.
	*/
	[[nodiscard]] const char* what() const noexcept override { return msg_.c_str(); }

	/**
	* Get the message of the exception.
	*/
	[[nodiscard]] const std::string& msg() const { return msg_; }

	/**
	* Get the module where the exception was thrown (may be empty).
	*/
	[[nodiscard]] const std::string& module() const { return module_; }

	/**
	* Get the name of the source file where the exception was thrown (may be empty).
	*/
	[[nodiscard]] const std::string& file() const { return file_; }

	/**
	* Get the line in the source file where the exception was thrown.
	*/
	[[nodiscard]] int line() const { return line_; }

private:

	//-----------------------------------------------------
	std::string msg_;                       ///< Message.
	std::string module_;                    ///< Module.
	std::string file_;                      ///< Source file.
	int line_ = 0;                          ///< Line in the source file.
};


}  //-------- End of namespace evocomb ---------------------------------------------


//------------------------------------------------------------------------------
// Macro to generate a RException. It takes the same values as the LOG_* macros (without the logger). As for these
// macros, the source file must declare its module with LOG_MODULE:
//   mThrowRException("Invalid ", idx, " index");
#define mThrowRException(...) throw evocomb::RException(evocomb_log_module_name, __FILE__, __LINE__, evocomb::RConcat::concat(__VA_ARGS__))
