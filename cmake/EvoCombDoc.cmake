# Generation of the documentation of EvoComb with Doxygen.
#
# The HTML documentation is generated in doc/html (one directory per library)
# during the build, and only if a header, a main page or a configuration file
# changed. Each library also generates a Doxygen tag file, used by the
# documentation of the libraries depending on it and by the projects using EvoComb
# (see EVOCOMB_DOXYGEN_TAGFILES in EvoCombConfig.cmake).
#
# Variables used: EVOCOMB_DOC_DIR (output directory).

find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(NOT DOXYGEN_FOUND)
    message(STATUS "Doxygen not found: the documentation is not generated")
    set(EVOCOMB_BUILD_DOC OFF)
    return()
endif()

# Configuration files
set(EVOCOMB_DOC_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/doc)
if(DOXYGEN_DOT_FOUND)
    set(EVOCOMB_DOC_HAVE_DOT YES)
    get_filename_component(EVOCOMB_DOC_DOT_PATH "${DOXYGEN_DOT_EXECUTABLE}" DIRECTORY)
else()
    set(EVOCOMB_DOC_HAVE_DOT NO)
    set(EVOCOMB_DOC_DOT_PATH "")
endif()
# Theme (doxygen-awesome-css). Its JavaScript extensions and the switch between
# the libraries must be loaded by the header of the pages: the default header
# of the installed Doxygen is generated, and the scripts are added at the end
# of its <head> (see evocomb_add_doc).
set(EVOCOMB_DOC_THEME_DIR ${PROJECT_SOURCE_DIR}/doc/doxygen-awesome)
set(EVOCOMB_DOC_DEFAULT_HEADER "")
file(MAKE_DIRECTORY ${EVOCOMB_DOC_BUILD_DIR})
execute_process(
        COMMAND ${DOXYGEN_EXECUTABLE} -w html default_header.html default_footer.html default_style.css
        WORKING_DIRECTORY ${EVOCOMB_DOC_BUILD_DIR}
        RESULT_VARIABLE result
        OUTPUT_QUIET ERROR_QUIET
)
if(result EQUAL 0 AND EXISTS ${EVOCOMB_DOC_BUILD_DIR}/default_header.html)
    file(READ ${EVOCOMB_DOC_BUILD_DIR}/default_header.html EVOCOMB_DOC_DEFAULT_HEADER)
else()
    message(STATUS "Doxygen cannot generate its default header: the theme is used without its extensions")
endif()

# Input filter putting the classes in the group of their directory
set(EVOCOMB_DOC_FILTER_SCRIPT ${PROJECT_SOURCE_DIR}/cmake/EvoCombDocFilter.cmake)

configure_file(${PROJECT_SOURCE_DIR}/doc/Doxyfile.common.in ${EVOCOMB_DOC_BUILD_DIR}/Doxyfile.common @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/doc/index.html.in ${EVOCOMB_DOC_DIR}/index.html @ONLY)

# Documentation of a library.
#   name        Name of the library (core or optimization).
#   DEPENDS     Tag files of the libraries used (the documentation is
#               regenerated when they change).
function(evocomb_add_doc name)
    cmake_parse_arguments(ARG "" "" "DEPENDS" ${ARGN})

    # Header of the pages: scripts of the theme and switch between the
    # libraries (the current one is highlighted)
    set(EVOCOMB_DOC_HTML_HEADER "")
    if(EVOCOMB_DOC_DEFAULT_HEADER)
        set(scripts [=[
<script type="text/javascript" src="$relpath^doxygen-awesome-darkmode-toggle.js"></script>
<script type="text/javascript" src="$relpath^doxygen-awesome-fragment-copy-button.js"></script>
<script type="text/javascript" src="$relpath^doxygen-awesome-paragraph-link.js"></script>
<script type="text/javascript" src="$relpath^doxygen-awesome-interactive-toc.js"></script>
<script type="text/javascript">
  DoxygenAwesomeDarkModeToggle.init()
  DoxygenAwesomeFragmentCopyButton.init()
  DoxygenAwesomeParagraphLink.init()
  DoxygenAwesomeInteractiveToc.init()
  // Switch between the libraries of EvoComb (under the project name)
  document.addEventListener("DOMContentLoaded", function() {
    var target = document.getElementById("projectalign") || document.getElementById("titlearea");
    if (!target) return;
    var nav = document.createElement("div");
    nav.id = "evocomb-libs";
    [["core", "core"], ["optimization", "optimization"]].forEach(function(lib) {
      var a = document.createElement("a");
      a.href = "$relpath^../" + lib[0] + "/index.html";
      a.textContent = lib[1];
      if (lib[0] === "@EVOCOMB_DOC_CURRENT@") a.className = "current";
      nav.appendChild(a);
    });
    target.appendChild(nav);
  });
</script>
</head>]=])
        string(REPLACE "@EVOCOMB_DOC_CURRENT@" "${name}" scripts "${scripts}")
        string(REPLACE "</head>" "${scripts}" header "${EVOCOMB_DOC_DEFAULT_HEADER}")
        file(WRITE ${EVOCOMB_DOC_BUILD_DIR}/header_${name}.html.tmp "${header}")
        # Only replace the header if it changed (to avoid regenerating the documentation)
        configure_file(${EVOCOMB_DOC_BUILD_DIR}/header_${name}.html.tmp ${EVOCOMB_DOC_BUILD_DIR}/header_${name}.html COPYONLY)
        set(EVOCOMB_DOC_HTML_HEADER ${EVOCOMB_DOC_BUILD_DIR}/header_${name}.html)
    endif()

    # Input filter (the file name is added by Doxygen)
    set(EVOCOMB_DOC_INPUT_FILTER "${CMAKE_COMMAND} -DEVOCOMB_DOC_ROOT=${PROJECT_SOURCE_DIR}/src/${name} -DEVOCOMB_DOC_PREFIX=${name} -DEVOCOMB_DOC_TMP=${EVOCOMB_DOC_BUILD_DIR} -P ${EVOCOMB_DOC_FILTER_SCRIPT}")

    configure_file(${PROJECT_SOURCE_DIR}/doc/${name}.Doxyfile.in ${EVOCOMB_DOC_BUILD_DIR}/${name}.Doxyfile @ONLY)
    file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/${name}/*.hpp)
    file(GLOB theme CONFIGURE_DEPENDS ${EVOCOMB_DOC_THEME_DIR}/*.css ${EVOCOMB_DOC_THEME_DIR}/*.js)
    set(tag ${EVOCOMB_DOC_DIR}/${name}/evocomb_${name}.tag)
    add_custom_command(
            OUTPUT ${tag}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${EVOCOMB_DOC_DIR}
            COMMAND ${DOXYGEN_EXECUTABLE} ${EVOCOMB_DOC_BUILD_DIR}/${name}.Doxyfile
            DEPENDS ${headers}
                    ${PROJECT_SOURCE_DIR}/doc/${name}.md
                    ${PROJECT_SOURCE_DIR}/doc/evocomb.dox
                    ${PROJECT_SOURCE_DIR}/doc/${name}_groups.dox
                    ${PROJECT_SOURCE_DIR}/doc/evocomb-theme.css
                    ${EVOCOMB_DOC_FILTER_SCRIPT}
                    ${EVOCOMB_DOC_BUILD_DIR}/${name}.Doxyfile
                    ${EVOCOMB_DOC_BUILD_DIR}/Doxyfile.common
                    ${EVOCOMB_DOC_HTML_HEADER}
                    ${theme}
                    ${ARG_DEPENDS}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMENT "Generating the documentation of evocomb::${name}"
            VERBATIM
    )
    set(EVOCOMB_DOC_TAG_${name} ${tag} PARENT_SCOPE)
endfunction()

evocomb_add_doc(core)
evocomb_add_doc(optimization DEPENDS ${EVOCOMB_DOC_TAG_core})

# Target built with "all" (the documentation is only regenerated if necessary)
add_custom_target(evocomb_doc ALL DEPENDS ${EVOCOMB_DOC_TAG_core} ${EVOCOMB_DOC_TAG_optimization})
