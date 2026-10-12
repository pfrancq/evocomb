# Doxygen input filter putting all the documented entities of a header in the
# group of its directory (for example core/matrix/r_matrix.hpp in the group
# core_matrix), so that the documentation lists the classes by directory.
#
# Usage (INPUT_FILTER, Doxygen adds the file name at the end):
#   cmake -DEVOCOMB_DOC_ROOT=<library directory> -DEVOCOMB_DOC_PREFIX=<library name>
#         -DEVOCOMB_DOC_TMP=<temporary directory> -P EvoCombDocFilter.cmake <file>
#
# The group is opened just after "namespace EvoComb {" and closed on the line of
# the end of the namespace, so that the line numbers are not modified. The
# groups are defined in doc/<library>_groups.dox.

math(EXPR last "${CMAKE_ARGC} - 1")
set(input "${CMAKE_ARGV${last}}")
file(READ "${input}" content)

file(RELATIVE_PATH rel "${EVOCOMB_DOC_ROOT}" "${input}")
if(input MATCHES "\\.hpp$" AND rel MATCHES "^([^/]+)/")
    set(group "${EVOCOMB_DOC_PREFIX}_${CMAKE_MATCH_1}")
    string(FIND "${content}" "\nnamespace evocomb" open)
    string(FIND "${content}" "End of namespace evocomb" close REVERSE)
    if(open GREATER_EQUAL 0 AND close GREATER open)
        # Close the group at the beginning of the line of the end of the namespace
        string(SUBSTRING "${content}" 0 ${close} before)
        string(FIND "${before}" "\n" lineStart REVERSE)
        math(EXPR lineStart "${lineStart} + 1")
        string(SUBSTRING "${content}" 0 ${lineStart} before)
        string(SUBSTRING "${content}" ${lineStart} -1 after)
        set(content "${before}/** @} */ ${after}")

        # Open the group after the brace of the namespace
        string(SUBSTRING "${content}" ${open} -1 after)
        string(FIND "${after}" "{" brace)
        math(EXPR brace "${open} + ${brace} + 1")
        string(SUBSTRING "${content}" 0 ${brace} before)
        string(SUBSTRING "${content}" ${brace} -1 after)
        set(content "${before} /** @addtogroup ${group} */ /** @{ */${after}")
    endif()
endif()

# Write the result on the standard output (through a temporary file to keep
# the content unchanged)
string(MD5 hash "${input}")
set(tmp "${EVOCOMB_DOC_TMP}/filter_${hash}.txt")
file(WRITE "${tmp}" "${content}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E cat "${tmp}")
file(REMOVE "${tmp}")
