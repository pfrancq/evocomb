/*

	EvoComb

	FileTools.cpp

	File tools - Implementation.

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
#include <cerrno>
#include <cstring>
#include <string>
#include <system_error>
#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <cstdlib>
    #include <unistd.h>
#endif


//------------------------------------------------------------------------------
// include files for EvoComb
#include "io/file_tools.hpp"
#include "io/r_io.hpp"
#include "app/r_logger.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");



//------------------------------------------------------------------------------
//
// class FileTools
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
std::filesystem::path FileTools::tempFile()
{
    std::error_code ec;
    const auto dir = std::filesystem::temp_directory_path(ec);
    if (ec)
        mThrowRIOException(nullptr, "Cannot find the temporary directory: ", ec.message());

#if defined(_WIN32)
    // GetTempFileNameW creates an empty file with a unique name
    wchar_t name[MAX_PATH];
    if (!GetTempFileNameW(dir.c_str(), L"evocomb", 0, name))
        mThrowRIOException(nullptr, "Cannot create a temporary file in ", dir.string(), ": ",
                           std::system_category().message(static_cast<int>(GetLastError())));
    return {name};
#else
    // mkstemp replaces the "XXXXXX" by a unique suffix and creates the file (atomically)
    std::string name = (dir / "evocomb.XXXXXX").string();
    const int fd = mkstemp(name.data());
    if (fd == -1)
        mThrowRIOException(nullptr, "Cannot create a temporary file in ", dir.string(), ": ", std::strerror(errno));
    close(fd);
    return {name};
#endif
}
