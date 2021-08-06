if(NOT PODOFO_MAIN_CMAKELISTS_READ)
    message(FATAL_ERROR "Run cmake on the CMakeLists.txt in the project root, not the one in the 'src' directory. You will need to delete CMakeCache.txt from the current directory.")
endif(NOT PODOFO_MAIN_CMAKELISTS_READ)

# Needed for <date/date.h>
add_definitions(-DHAS_UNCAUGHT_EXCEPTIONS=1)

set(PODOFO_BASE_DEPEND_TARGET CACHE INTERNAL
    "Which PoDoFo library target to depend on when building tools and tests")
set(PODOFO_DEPEND_TARGET CACHE INTERNAL
    "Which PoDoFo library target to depend on when building tools and tests")

set(PODOFO_BASE_HEADERS
    podofo-base.h
    podofo.h
    ${PROJECT_BINARY_DIR}/podofo_config.h
)
install(FILES ${PODOFO_BASE_HEADERS} DESTINATION "include/podofo")

set(PODOFO_SOURCES ${PODOFO_BASE_HEADERS})
if(WIN32 AND PODOFO_BUILD_SHARED)
    # If we build for windows systems, we also include the resource file
    list(APPEND PODOFO_SOURCES podofo.rc)
endif()

# Set visual studio source group
source_group("" FILES ${PODOFO_SOURCES})

foreach(subdir compat base extra private)
    # Add subdir to source files
    file(GLOB SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${subdir}/*")
    source_group(${subdir} FILES ${SOURCE_FILES})
    list(APPEND PODOFO_SOURCES ${SOURCE_FILES})
    if (NOT "${subdir}" STREQUAL "private")
        # Set non private headers for install
        set(HEADER_FILES ${SOURCE_FILES})
        list(FILTER HEADER_FILES EXCLUDE REGEX "\.cpp$")
        install(FILES ${HEADER_FILES} DESTINATION "include/podofo/${subdir}")
    endif()
endforeach()

if(NOT PODOFO_HAVE_FONTCONFIG)
    # Remove FontConfig source if not needed
    list(REMOVE_ITEM PODOFO_SOURCES
        base/PdfFontConfigWrapper.h
        base/PdfFontConfigWrapper.cpp
    )
endif()

if(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)
    message(FATAL_ERROR "At least one of PODOFO_BUILD_SHARED and PODOF_BUILD_STATIC must be set")
endif(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)

if(PODOFO_BUILD_STATIC)
    message("Building static PoDoFo library")
    add_library(podofo_static STATIC ${PODOFO_SOURCES})
    target_link_libraries(podofo_static ${PODOFO_LIB_DEPENDS})
    set_target_properties(podofo_static PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
        COMPILE_FLAGS "-DBUILDING_PODOFO"
    )
    set(PODOFO_DEPEND_TARGET podofo_static
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    set(USING_SHARED_PODOFO FALSE)
    install(TARGETS podofo_static
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "${LIBDIRNAME}"
        ARCHIVE DESTINATION "${LIBDIRNAME}"
    )
endif(PODOFO_BUILD_STATIC)

if(PODOFO_BUILD_SHARED)
    message("Building shared PoDoFo library")
    add_library(podofo_shared SHARED ${PODOFO_SOURCES})
    target_link_libraries(podofo_shared ${PODOFO_LIB_DEPENDS})
    # TODO: set /wd4251 flag if we're doing a debug build with
    # Visual Studio, since it produces invalid warnings about STL
    # use.
    set_target_properties(podofo_shared PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
        COMPILE_FLAGS "-DBUILDING_PODOFO"
    )
    # Since we're building a shared podofo, prefer to depend on this one for
    # tests and tools over the static library (if built).
    set(PODOFO_DEPEND_TARGET podofo_shared
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    set(USING_SHARED_PODOFO TRUE)
    install(TARGETS podofo_shared
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
            "libpodofo.pc.in"
            "${PROJECT_BINARY_DIR}/libpodofo.pc"
            @ONLY)
        install(
            FILES "${PROJECT_BINARY_DIR}/libpodofo.pc"
            DESTINATION "${LIBDIRNAME}/pkgconfig")
    else(PKG_CONFIG_FOUND)
        message("Pkg-config not found. No pkg-config file will be created.")
    endif(PKG_CONFIG_FOUND)
endif(PODOFO_BUILD_SHARED)

# Use these flags when compiling code that includes PoDoFo headers.
# Failure to do so will result in compilation or link-time errors
# on some platforms, and can even cause undefined results at runtime.
if(WIN32 AND USING_SHARED_PODOFO)
    set(PODOFO_CFLAGS "-DUSING_SHARED_PODOFO" CACHE INTERNAL "Extra flags required when linking to PoDoFo")
else(WIN32 AND USING_SHARED_PODOFO)
    set(PODOFO_CFLAGS "" CACHE INTERNAL "Extra flags required when linking to PoDoFo")
endif(WIN32 AND USING_SHARED_PODOFO)

# Write the cflags and depend target to the config file
file(APPEND
    "${PROJECT_BINARY_DIR}/PoDoFoConfig.cmake"
    "set(PODOFO_CFLAGS ${PODOFO_CFLAGS})\n"
)
file(APPEND
    "${PROJECT_BINARY_DIR}/PoDoFoConfig.cmake"
    "set(PODOFO_DEPEND_TARGET ${PODOFO_DEPEND_TARGET})\n"
)
