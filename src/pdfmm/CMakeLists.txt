if(NOT PDFMM_MAIN_CMAKELISTS_READ)
    message(FATAL_ERROR "Run cmake on the CMakeLists.txt in the project root, not the one in the 'src' directory. You will need to delete CMakeCache.txt from the current directory.")
endif()

# Generate our configure file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pdfmm_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/pdfmm_config.h)

# Needed for <date/date.h>
add_definitions(-DHAS_UNCAUGHT_EXCEPTIONS=1)

set(PDFMM_INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/3rdparty/include"
    CACHE INTERNAL "pdfmm include directories"
)

include_directories(
    ${PDFMM_INCLUDE_DIRS}
    ${PDFMM_HEADERS_DEPENDS}
)

set(PDFMM_BASE_HEADERS
    pdfmm-base.h
    pdfmm.h
    ${CMAKE_CURRENT_BINARY_DIR}/pdfmm_config.h
)
install(FILES ${PDFMM_BASE_HEADERS} DESTINATION "include/pdfmm")

set(PDFMM_SOURCES ${PDFMM_BASE_HEADERS})
if(WIN32 AND PDFMM_BUILD_SHARED)
    # If we build for windows systems, we also include the resource file
    list(APPEND PDFMM_SOURCES pdfmm.rc)
endif()

# Set visual studio source group
source_group("" FILES ${PDFMM_SOURCES})

foreach(subdir compat base contrib private)
    file(GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${subdir}/*")
    # Set visual studio source group
    source_group(${subdir} FILES ${SOURCE_FILES})
    # Add subdir to source files
    list(APPEND PDFMM_SOURCES ${SOURCE_FILES})
    if (NOT "${subdir}" STREQUAL "private")
        # Set non private headers for install
        set(HEADER_FILES ${SOURCE_FILES})
        list(FILTER HEADER_FILES EXCLUDE REGEX "\.cpp$")
        install(FILES ${HEADER_FILES} DESTINATION "include/pdfmm/${subdir}")
    endif()
endforeach()

if(NOT PDFMM_HAVE_FONTCONFIG)
    # Remove FontConfig source if not needed
    list(REMOVE_ITEM PDFMM_SOURCES
        base/PdfFontConfigWrapper.h
        base/PdfFontConfigWrapper.cpp
    )
endif()

if(NOT PDFMM_BUILD_SHARED AND NOT PDFMM_BUILD_STATIC)
    message(FATAL_ERROR "At least one of PDFMM_BUILD_SHARED and PODOF_BUILD_STATIC must be set")
endif()

if(PDFMM_BUILD_STATIC)
    message("Building static pdfmm library")
    add_library(pdfmm_static STATIC ${PDFMM_SOURCES})
    target_link_libraries(pdfmm_static ${PDFMM_LIB_DEPENDS})
    set_target_properties(pdfmm_static PROPERTIES
        VERSION "${PDFMM_LIBVERSION}"
        SOVERSION "${PDFMM_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "pdfmm"
        COMPILE_FLAGS "-DBUILDING_PDFMM"
    )
    set(USING_SHARED_PDFMM FALSE)
    install(TARGETS pdfmm_static
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "${LIBDIRNAME}"
        ARCHIVE DESTINATION "${LIBDIRNAME}"
    )
    set(PDFMM_LIBRARIES pdfmm_static
        CACHE INTERNAL "Which pdfmm library variant to depend on")
endif()

if(PDFMM_BUILD_SHARED)
    message("Building shared pdfmm library")
    add_library(pdfmm_shared SHARED ${PDFMM_SOURCES})
    target_link_libraries(pdfmm_shared ${PDFMM_LIB_DEPENDS})
    # TODO: set /wd4251 flag if we're doing a debug build with
    # Visual Studio, since it produces invalid warnings about STL
    # use.
    set_target_properties(pdfmm_shared PROPERTIES
        VERSION "${PDFMM_LIBVERSION}"
        SOVERSION "${PDFMM_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "pdfmm"
        COMPILE_FLAGS "-DBUILDING_PDFMM"
    )
    # Since we're building a shared pdfmm, prefer to depend on this one for
    # tests and tools over the static library (if built).
    set(USING_SHARED_PDFMM TRUE)
    install(TARGETS pdfmm_shared
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "${LIBDIRNAME}"
        ARCHIVE DESTINATION "${LIBDIRNAME}"
    )

    # Create a pkg-config file for linking against shared library
    # if pkg-config is available on the system.
    # Add a version to the file name corresponding to the API compatibility.

    find_program(PKG_CONFIG_FOUND pkg-config)
    if(PKG_CONFIG_FOUND)
        message("Pkg-config found, creating a pkg-config file for linking against shared library.")
        configure_file(
            "libpdfmm.pc.in"
            "${PROJECT_BINARY_DIR}/libpdfmm.pc"
            @ONLY)
        install(
            FILES "${PROJECT_BINARY_DIR}/libpdfmm.pc"
            DESTINATION "${LIBDIRNAME}/pkgconfig")
    else()
        message("Pkg-config not found. No pkg-config file will be created.")
    endif()

    set(PDFMM_LIBRARIES pdfmm_shared
        CACHE INTERNAL "Which pdfmm library variant to depend on")
endif()

# Use these flags when compiling code that includes pdfmm headers.
# Failure to do so will result in compilation or link-time errors
# on some platforms, and can even cause undefined results at runtime.
if(WIN32 AND USING_SHARED_PDFMM)
    set(PDFMM_CFLAGS "-DUSING_SHARED_PDFMM" CACHE INTERNAL "Extra flags required when linking to pdfmm")
else()
    set(PDFMM_CFLAGS "" CACHE INTERNAL "Extra flags required when linking to pdfmm")
endif()

# Write the cflags and depend target to the config file
file(APPEND
    "${PROJECT_BINARY_DIR}/pdfmmConfig.cmake"
    "set(PDFMM_CFLAGS ${PDFMM_CFLAGS})\n"
)
file(APPEND
    "${PROJECT_BINARY_DIR}/pdfmmConfig.cmake"
    "set(PDFMM_LIBRARIES ${PDFMM_LIBRARIES})\n"
)
