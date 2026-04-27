/*

	EvoComb

	FileTools.hpp

	File tools - Header.

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
#include <filesystem>


//------------------------------------------------------------------------------
namespace evocomb{
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The FileTools class provides static methods to manage files that are not provided by std::filesystem (use
 * std::filesystem::remove or std::filesystem::rename to remove or rename a file).
 *
 * It is a static class.
 * @short File tools.
 */
class FileTools
{
public:

    /**
     * Create a new empty temporary file with a unique name in the temporary directory of the system (see
     * std::filesystem::temp_directory_path). Since the file is created, the name cannot be returned by another call
     * (even from another process). The file is not removed automatically: it must be removed by the caller when it is
     * not needed anymore (see also RSmartTempFile).
     * @return Path of the file.
     * @exception RIOException          The file cannot be created.
     */
    [[nodiscard]] static std::filesystem::path tempFile();

    //-----------------------------------------------------
    FileTools() = delete;
};


}  //-------- End of namespace evocomb ---------------------------------------------
