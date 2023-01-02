cmake_minimum_required(VERSION 3.16)

project(pdfmm)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.1)
        message(FATAL_ERROR "GCC version must be at least 8.1")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        message(FATAL_ERROR "Clang version must be at least 7.0")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.9")
        message(FATAL_ERROR "MSVC version bust be at least 15.9")
    endif()
else()
    message(WARNING "You are using an unsupported compiler!")
endif()

set(PDFMM_VERSION_MAJOR "0" CACHE STRING "Major part of pdfmm version number")
set(PDFMM_VERSION_MINOR "10" CACHE STRING "Minor part of pdfmm version number")
set(PDFMM_VERSION_PATCH "0" CACHE STRING "Patchlevel part of pdfmm version number")
set(PDFMM_SOVERSION "${PDFMM_VERSION_MAJOR}.${PDFMM_VERSION_MINOR}.${PDFMM_VERSION_PATCH}")
set(PDFMM_LIBVERSION "${PDFMM_SOVERSION}")

#
# Main includes
#
include(CheckIncludeFile)
include(CheckLibraryExists)
include(TestBigEndian)
include(CheckTypeSize)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)

# Load modules from our source tree too
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Builds must use this CMakeLists.txt, not the one in src/ or somewhere else.
# If users try to use something else the results can be confusing. We set a
# variable here that we require to be set elsewhere, otherwise we'll complain.
set(PDFMM_MAIN_CMAKELISTS_READ TRUE)

if(NOT PDFMM_BUILD_LIB_ONLY)
    # We assume a standalone build so we set output
    # path to a fixed location
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target)
endif()

option(PDFMM_SHARED "Build shared libraries (.dll/.so)" ON)
if(PDFMM_SHARED)
    add_compile_definitions(PDFMM_SHARED)
else()
    set(PDFMM_STATIC TRUE)
    add_compile_definitions(PDFMM_STATIC)
endif()

message("Will install libraries to ${CMAKE_INSTALL_FULL_LIBDIR}")

# Linux packagers want an uninstall target.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}"
    -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

if(WIN32)
    # We must explicitly link to the core win32 libraries
    set(PLATFORM_SYSTEM_LIBRARIES kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid ws2_32)
else()
    set(PLATFORM_SYSTEM_LIBRARIES)
endif()

if (MSVC)
    # Set the compiler less permissive and add some warnings
    # /w34388: 'expression': signed/unsigned mismatch
    # /w34389: 'operator': signed/unsigned mismatch
    # /we4287: unsigned/negative constant mismatch
    # /w34100: unreferenced formal parameter
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive- /w34388 /w34389 /we4287 /w34100 /Zc:__cplusplus")

    # Microsoft deprecate certain POSIX functions that we use.
    # for now, turn off these warnings.
    add_compile_definitions(_CRT_SECURE_NO_DEPRECATE)
else()
    message("Using gcc/clang specific compiler options")

    # By default hide symbols on gcc/clang
    add_compile_options(-fvisibility=hidden)

    # TODO: Remove -Wno-format-truncation after porting the
    # code to std::fmt as it will be unuseful
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Woverloaded-virtual -Wswitch -Wcast-qual -Wwrite-strings -Wredundant-decls -Wreorder -Wno-format-truncation -Wno-unknown-pragmas -Wno-unknown-warning-option")

    # Note that we do not need debug definitions here. Set
    # -DCMAKE_BUILD_TYPE=debug or (if you want an optimised
    # release build with debug info) -DCMAKE_CXX_FLAGS="-g3"
    #
    # We add -W unless we're using gcc on win32, where it produces
    # spurious warnings about dllimport of inlines because of a dllimport
    # declaration on the whole class.
    if(NOT WIN32)
        add_definitions(-W)
    endif()
endif()

find_package(ZLIB REQUIRED)
message("Found zlib headers in ${ZLIB_INCLUDE_DIR}, library at ${ZLIB_LIBRARIES}")

find_package(OpenSSL REQUIRED)

message("OPENSSL_VERSION: ${OPENSSL_LIBRARIES}")

find_package(Libidn)

if(Libidn_FOUND)
    message("Found Libidn headers in ${Libidn_INCLUDE_DIR}, library at ${Libidn_LIBRARIES}")
    set(PDFMM_HAVE_LIBIDN TRUE)
    message("Libidn found. AES-256 Encryption support will be enabled")
else()
    message("Libidn not found. AES-256 Encryption support will be disabled")
endif()

find_package(JPEG)

if(JPEG_FOUND)
    message("Found libjpeg headers in ${JPEG_INCLUDE_DIR}, library at ${JPEG_LIBRARIES}")
    set(PDFMM_HAVE_JPEG_LIB TRUE)
else()
    message("Libjpeg not found. JPEG support will be disabled")
endif()

find_package(TIFF)

if(TIFF_FOUND)
    message("Found libtiff headers in ${TIFF_INCLUDE_DIR}, library at ${TIFF_LIBRARIES}")
    set(PDFMM_HAVE_TIFF_LIB TRUE)
else()
    message("Libtiff not found. TIFF support will be disabled")
endif()

find_package(PNG)

if(PNG_FOUND)
    message("Found LibPng headers in ${PNG_INCLUDE_DIR}, library at ${PNG_LIBRARIES}")
    set(PDFMM_HAVE_PNG_LIB TRUE)
else()
    message("LibPng not found. PNG support will be disabled")
    set(PNG_LIBRARIES "")
endif()

find_package(Freetype REQUIRED)
message("Found freetype library at ${FREETYPE_LIBRARIES}, headers ${FREETYPE_INCLUDE_DIRS}")

find_package(Fontconfig)
if(Fontconfig_FOUND)
    set(PDFMM_HAVE_FONTCONFIG TRUE)
    message("Found fontconfig headers in ${Fontconfig_INCLUDE_DIRS}, library at ${Fontconfig_LIBRARIES}")
elseif(WIN32)
    # Fallback to GDI if FontConfig is missing in Windows
    set(PDFMM_HAVE_WIN32GDI TRUE)
endif()

find_package(LibXml2 REQUIRED)
message("Found libxml2 library at ${LIBXML2_LIBRARIES}, headers ${LIBXML2_INCLUDE_DIRS}")

# The pdfmm library needs to be linked to these libraries
# NOTE: Be careful when adding/removing: the order may be
# platform sensible, so don't modify the current order
set(PDFMM_LIB_DEPENDS
    ${OPENSSL_LIBRARIES}
    ${Fontconfig_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${PNG_LIBRARIES}
    ${TIFF_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PLATFORM_SYSTEM_LIBRARIES}
)

set(PDFMM_HEADERS_DEPENDS
    ${OPENSSL_INCLUDE_DIR}
    ${Fontconfig_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIR}
    ${TIFF_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIR} 
    ${ZLIB_INCLUDE_DIR}
    ${PLATFORM_SYSTEM_HEADERS}
)

if(Libidn_FOUND)
    list(APPEND PDFMM_LIB_DEPENDS ${Libidn_LIBRARIES})
    list(APPEND PDFMM_HEADERS_DEPENDS ${Libidn_INCLUDE_DIR})
endif()

# Create the config file. It'll be appended to as the subdirs run though
# then dependency information will be written to it at the end of the
# build.
file(WRITE
    "${PROJECT_BINARY_DIR}/pdfmmConfig.cmake"
    "# CMake module for pdfmm\n"
)
file(APPEND
    "${PROJECT_BINARY_DIR}/pdfmmConfig.cmake"
    "set(PDFMM_INCLUDES ${PROJECT_SOURCE_DIR}/src)\n"
)

add_subdirectory(src/pdfmm)
include_directories(${PDFMM_INCLUDE_DIRS})

if(NOT PDFMM_BUILD_LIB_ONLY)
    enable_testing()
    add_subdirectory(test)
    add_subdirectory(examples)
endif()

# Export some variables into the config file so it's easier for others
# to build and link against pdfmm

# To use these dependencies set PDFMM_DIR to the pdfmm BUILD directory in
# your build (eg -DPDFMM_DIR=/path/to/pdfmm when running cmake to configure
# the app that'll use pdfmm). See: find_package(...) in the cmake docs.
if(PDFMM_SHARED)
    export(TARGETS pdfmm_shared FILE "${PROJECT_BINARY_DIR}/pdfmmConfig.cmake")
else()
    export(TARGETS pdfmm_static pdfmm_private FILE "${PROJECT_BINARY_DIR}/pdfmmConfig.cmake")
endif()

# Enable packaging
set(CPACK_PACKAGE_DESCRIPTION "A C++ PDF manipulation library forked from PoDoFo")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/pdfmm/pdfmm")
set(CPACK_PACKAGE_VENDOR "Francesco Pretto")
set(CPACK_PACKAGE_CONTACT "ceztko@gmail.com")
set(CPACK_PACKAGE_VERSION ${PDFMM_LIBVERSION})
if (WIN32)
    # Set MSI generation
    set(CPACK_GENERATOR "WIX")
endif()
include(CPack)
