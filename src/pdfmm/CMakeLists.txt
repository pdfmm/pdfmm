if(NOT PDFMM_MAIN_CMAKELISTS_READ)
    message(FATAL_ERROR "Run cmake on the CMakeLists.txt in the project root, not the one in the 'src' directory. You will need to delete CMakeCache.txt from the current directory.")
endif()

# Generate our configure file
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/pdfmm_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/pdfmm_config.h")

# Needed for <date/date.h>
add_compile_definitions(HAS_UNCAUGHT_EXCEPTIONS=1)

set(PDFMM_INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/3rdparty"
    CACHE INTERNAL "pdfmm include directories"
)

include_directories(
    ${PDFMM_INCLUDE_DIRS}
    ${PDFMM_HEADERS_DEPENDS}
)

add_subdirectory(private)

set(PDFMM_BASE_HEADERS
    pdfmm-base.h
    pdfmm-contrib.h
    pdfmm.h
)
set(PDFMM_CONFIG_HEADER "${CMAKE_CURRENT_BINARY_DIR}/pdfmm_config.h")

install(FILES ${PDFMM_BASE_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/pdfmm")
install(FILES ${PDFMM_CONFIG_HEADER} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/pdfmm/base")

set(PDFMM_SOURCES ${PDFMM_BASE_HEADERS} ${PDFMM_CONFIG_HEADER})
if(PDFMM_SHARED AND WIN32)
    # If we build for windows systems, we also include the resource file
    list(APPEND PDFMM_SOURCES pdfmm.rc)
endif()

# Set visual studio source group
source_group("" FILES ${PDFMM_SOURCES})

macro(add_source subdir subdir_path)
    file(GLOB_RECURSE SOURCE_FILES "${subdir_path}/*")
    # Set visual studio source group
    source_group(${subdir} FILES ${SOURCE_FILES})
    # Add subdir to source files
    list(APPEND PDFMM_SOURCES ${SOURCE_FILES})
    # Set non private headers for install
    set(HEADER_FILES ${SOURCE_FILES})
    list(FILTER HEADER_FILES EXCLUDE REGEX "\.cpp$")
endmacro()

foreach(subdir common base contrib)
    add_source(${subdir} "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")
    install(FILES ${HEADER_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/pdfmm/${subdir}")
endforeach()

if(NOT PDFMM_HAVE_FONTCONFIG)
    # Remove FontConfig source if not needed
    list(REMOVE_ITEM PDFMM_SOURCES
        base/PdfFontConfigWrapper.h
        base/PdfFontConfigWrapper.cpp
    )
endif()

add_source("3rdparty/pdfium" "${PROJECT_SOURCE_DIR}/3rdparty/pdfium")

if(PDFMM_STATIC)
    message("Building static pdfmm library")
    add_library(pdfmm_static STATIC ${PDFMM_SOURCES})
    target_link_libraries(pdfmm_static pdfmm_private ${PDFMM_LIB_DEPENDS})
    set_target_properties(pdfmm_static PROPERTIES
        VERSION "${PDFMM_LIBVERSION}"
        SOVERSION "${PDFMM_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "pdfmm"
    )
    install(TARGETS pdfmm_static
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    set(PDFMM_LIBRARIES pdfmm_static pdfmm_private
        CACHE INTERNAL "Which pdfmm library variant to depend on")
endif()

if(PDFMM_SHARED)
    message("Building shared pdfmm library")
    add_library(pdfmm_shared SHARED ${PDFMM_SOURCES})
    target_link_libraries(pdfmm_shared PRIVATE pdfmm_private PRIVATE ${PDFMM_LIB_DEPENDS})
    # TODO: set /wd4251 flag if we're doing a debug build with
    # Visual Studio, since it produces invalid warnings about STL
    # use.
    set_target_properties(pdfmm_shared PROPERTIES
        VERSION "${PDFMM_LIBVERSION}"
        SOVERSION "${PDFMM_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "pdfmm"
    )
    # Since we're building a shared pdfmm, prefer to depend on this one for
    # tests and tools over the static library (if built).
    install(TARGETS pdfmm_shared
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
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
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    else()
        message("Pkg-config not found. No pkg-config file will be created.")
    endif()

    set(PDFMM_LIBRARIES pdfmm_shared
        CACHE INTERNAL "Which pdfmm library variant to depend on")
endif()

# Use these flags when compiling code that includes pdfmm headers.
# Failure to do so will result in compilation or link-time errors
# on some platforms, and can even cause undefined results at runtime.
if(STATIC_PDFMM)
    set(PDFMM_CFLAGS "-DPDFMM_STATIC" CACHE INTERNAL "Extra flags required when linking to pdfmm")
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
