cmake_minimum_required(VERSION 3.16)

#*****************  IMPORTANT  ************* IMPORTANT **********************
# Look at http://www.vtk.org/Wiki/CMake_HowToDoPlatformChecks
# and the other wiki entries before you add anything. You might not need to.
#****************************************************************************

#
# Project name and version
#
project(PoDoFo)

set(PODOFO_VERSION_MAJOR "0" CACHE STRING "Major part of PoDoFo version number")
set(PODOFO_VERSION_MINOR "9" CACHE STRING "Minor part of PoDoFo version number")
set(PODOFO_VERSION_PATCH "6" CACHE STRING "Patchlevel part of PoDoFo version number")
set(PODOFO_SOVERSION "${PODOFO_VERSION_MAJOR}.${PODOFO_VERSION_MINOR}.${PODOFO_VERSION_PATCH}")
set(PODOFO_LIBVERSION "${PODOFO_SOVERSION}")


#
# Main includes
#
include(CheckIncludeFile)
include(CheckLibraryExists)
include(TestBigEndian)
include(CheckTypeSize)

set(CMAKE_CXX_STANDARD 17)

# Load modules from our source tree too
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")


# Builds must use this CMakeLists.txt, not the one in src/ or somewhere else.
# If users try to use something else the results can be confusing. We set a
# variable here that we require to be set elsewhere, otherwise we'll complain.
set(PODOFO_MAIN_CMAKELISTS_READ TRUE)

# If the user hasn't told use specifically what they want, build only
# a static library or only a shared library on Windows.
if(NOT DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
    if(WIN32)
        set(PODOFO_BUILD_STATIC FALSE)
        set(PODOFO_BUILD_SHARED TRUE)
    else(WIN32)
        set(PODOFO_BUILD_STATIC TRUE)
        set(PODOFO_BUILD_SHARED FALSE)
    endif(WIN32)
endif(NOT DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
if(DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
    if(PODOFO_BUILD_SHARED)
        set(PODOFO_BUILD_STATIC FALSE)
    else(PODOFO_BUILD_SHARED)
        set(PODOFO_BUILD_STATIC TRUE)
    endif(PODOFO_BUILD_SHARED)
endif(DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
if(NOT DEFINED PODOFO_BUILD_SHARED AND DEFINED PODOFO_BUILD_STATIC)
    if(PODOFO_BUILD_STATIC)
        set(PODOFO_BUILD_SHARED FALSE)
    else(PODOFO_BUILD_STATIC)
        set(PODOFO_BUILD_SHARED TRUE)
    endif(PODOFO_BUILD_STATIC)
endif(NOT DEFINED PODOFO_BUILD_SHARED AND DEFINED PODOFO_BUILD_STATIC)

if(DEFINED LIB_SUFFIX)
    set(LIBDIRNAME "lib${LIB_SUFFIX}")
else(DEFINED LIB_SUFFIX)

# Some 64 bit linux distros use /usr/lib64 for 64 bit libraries.
# on these platforms we must
if(NOT DEFINED WANT_LIB64)
    # TODO: detect 64-bit build and existance of /usr/lib64 and set to TRUE.
    message("WANT_LIB64 unset; assuming normal library directory names")
    set(WANT_LIB64 FALSE)
endif(NOT DEFINED WANT_LIB64)

if(WANT_LIB64)
    set(LIBDIRNAME "lib64")
else(WANT_LIB64)
    set(LIBDIRNAME "lib")
endif(WANT_LIB64)

endif(DEFINED LIB_SUFFIX)
message("Will install libraries to ${CMAKE_INSTALL_PREFIX}/${LIBDIRNAME}")

# Linux packagers want an uninstall target.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}"
    -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Check if we are big endian
TEST_BIG_ENDIAN(TEST_BIG)

if(WIN32)
    # We must explicitly link to the core win32 libraries
    set(PLATFORM_SYSTEM_LIBRARIES kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid ws2_32)
    # Microsoft deprecate certain POSIX functions that we use.
    # for now, turn off these warnings.
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
else(WIN32)
    set(PLATFORM_SYSTEM_LIBRARIES)
endif(WIN32)

if(UNIX)
    set(WANT_FONTCONFIG TRUE CACHE INTERNAL
        "True if PoDoFo should be built with fontconfig support")
else()
    set(WANT_FONTCONFIG FALSE CACHE INTERNAL
        "True if PoDoFo should be built with fontconfig support")
endif()


if(CMAKE_COMPILER_IS_GNUCXX)
    message("Using gcc specific compiler options")
    # We can be more specific about what we want out of g++
    # than with most other compilers.

    # If the user hasn't specifically said whether they want
    # -fvisibility=hidden or not, turn it on if it's said to
    # be supported, off for other gcc versions.
    if(NOT DEFINED PODOFO_USE_VISIBILITY)
        set(PODOFO_USE_VISIBILITY ${PODOFO_HAVE_GCC_SYMBOL_VISIBILITY})
    endif(NOT DEFINED PODOFO_USE_VISIBILITY)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Woverloaded-virtual -Wswitch -Wcast-qual -Wwrite-strings -Wredundant-decls -Wreorder -Wno-deprecated-declarations")

    #
    # Note that we do not need debug definitions here. Set
    # -DCMAKE_BUILD_TYPE=debug or (if you want an optimised
    # release build with debug info) -DCMAKE_CXX_FLAGS="-g3"
    #
    # We add -W unless we're using gcc on win32, where it produces
    # spurious warnings about dllimport of inlines because of a dllimport
    # declaration on the whole class.
    if(NOT WIN32)
        add_definitions(-W)
    endif(NOT WIN32)
    # If they've enabled the use of gcc4 symbol visibility, use it.
    if(PODOFO_USE_VISIBILITY)
        add_definitions(
            -DPODOFO_HAVE_GCC_SYMBOL_VISIBILITY
            -fvisibility=hidden
            )
    endif(PODOFO_USE_VISIBILITY)
endif(CMAKE_COMPILER_IS_GNUCXX)

find_package(ZLIB REQUIRED)
message("Found zlib headers in ${ZLIB_INCLUDE_DIR}, library at ${ZLIB_LIBRARIES}")

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

message("OPENSSL_VERSION: ${OPENSSL_LIBRARIES}")

find_package(LIBIDN)

if(LIBIDN_FOUND)
    message("Found libidn headers in ${LIBIDN_INCLUDE_DIR}, library at ${LIBIDN_LIBRARIES}")
endif(LIBIDN_FOUND)

if(LIBIDN_FOUND)
    set(PODOFO_HAVE_LIBIDN TRUE)
    include_directories(${LIBIDN_INCLUDE_DIR})
    message("Libidn found. AES-256 Encryption support will be enabled")
else(LIBIDN_FOUND)
    message("Libidn not found. AES-256 Encryption support will be disabled")
endif(LIBIDN_FOUND)

find_package(JPEG)

if(JPEG_FOUND)
    message("Found libjpeg headers in ${JPEG_INCLUDE_DIR}, library at ${JPEG_LIBRARIES}")
    set(PODOFO_HAVE_JPEG_LIB TRUE)
    include_directories(${JPEG_INCLUDE_DIR})
else(JPEG_FOUND)
    message("Libjpeg not found. JPEG support will be disabled")
endif(JPEG_FOUND)

find_package(TIFF)

if(TIFF_FOUND)
    message("Found libtiff headers in ${TIFF_INCLUDE_DIR}, library at ${TIFF_LIBRARIES}")
    set(PODOFO_HAVE_TIFF_LIB TRUE)
    include_directories(${TIFF_INCLUDE_DIR})
else(TIFF_FOUND)
    message("Libtiff not found. TIFF support will be disabled")
endif(TIFF_FOUND)

find_package(PNG)

if(PNG_FOUND)
    message("Found LibPng headers in ${PNG_INCLUDE_DIR}, library at ${PNG_LIBRARIES}")
    set(PODOFO_HAVE_PNG_LIB TRUE)
    include_directories(${PNG_INCLUDE_DIR})
else(PNG_FOUND)
    message("LibPng not found. PNG support will be disabled")
    set(PNG_LIBRARIES "")
endif(PNG_FOUND)

find_package(Freetype REQUIRED)
message("Found freetype library at ${FREETYPE_LIBRARIES}, headers ${FREETYPE_INCLUDE_DIRS}")

if(WANT_FONTCONFIG)
    find_package(Fontconfig REQUIRED)
    set(PODOFO_HAVE_FONTCONFIG TRUE)
    set(PODOFO_LIB_FONTCONFIG:STRING fontconfig)
    if(FONTCONFIG_FOUND)
        message("Found fontconfig headers in ${FONTCONFIG_INCLUDE_DIR}, library at ${Fontconfig_LIBRARIES}")
    else(FONTCONFIG_FOUND)
        message("Could not find fontconfig.")
    endif(FONTCONFIG_FOUND)
else(WANT_FONTCONFIG)
    # Might as well look for it anyway. This also sets the appropriate
    # variables to empty values.
    find_package(Fontconfig)
    set(PODOFO_LIB_FONTCONFIG:STRING)
endif(WANT_FONTCONFIG)

if(WANT_BOOST)
    message("Looking optional for Boost.")
    message("Boost is optional, so don't worry if it is not found.")
    message("Set the BOOST_ROOT env var if you have problems.")
    find_package(Boost)
    if(BOOST_FOUND)
        set(PODOFO_HAVE_BOOST TRUE)
        include_directories(${BOOST_INCLUDE_DIR})
    else(BOOST_FOUND)
        message("If you don't need graph support you can ignore the above error.")
    endif(BOOST_FOUND)
endif(WANT_BOOST)

include_directories(BEFORE     # before toolchain include dir (to ignore installed)
    ${PoDoFo_SOURCE_DIR}/src   # order will be reversed, so this is the second dir
    ${PoDoFo_BINARY_DIR}       # because of BEFORE, this is the first include dir
)

include_directories(
    ${PoDoFo_SOURCE_DIR}/src/podofo
    ${PoDoFo_SOURCE_DIR}/3rdparty/include
    ${FREETYPE_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
)
LINK_DIRECTORIES(${PoDoFo_BINARY_DIR}/src/podofo)

#
# The PoDoFo library needs to be linked to these libraries,
# as do any apps or libraries linking to PoDoFo. PODOFO_LIB
# will include these and the correct podofo target, so clients
# should specify only PODOFO_LIB .
#
set(PODOFO_LIB_DEPENDS
    ${LIBIDN_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${PNG_LIBRARIES}
    ${TIFF_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PLATFORM_SYSTEM_LIBRARIES}
)

if(FONTCONFIG_FOUND AND WANT_FONTCONFIG)
    set(PODOFO_LIB_DEPENDS ${Fontconfig_LIBRARIES} ${PODOFO_LIB_DEPENDS})
    include_directories(${Fontconfig_INCLUDE_DIRS})
endif(FONTCONFIG_FOUND AND WANT_FONTCONFIG)

if(WIN32 OR PODOFO_BUILD_STATIC)
    set(PODOFO_LIB
        podofo
        ${PODOFO_LIB_DEPENDS}
    )
else(WIN32 OR PODOFO_BUILD_STATIC)
    set(PODOFO_LIB podofo)
endif(WIN32 OR PODOFO_BUILD_STATIC)

# Create the config file. It'll be appended to as the subdirs run though
# then dependency information will be written to it at the end of the
# build.
file(WRITE
    "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
    "# CMake module for PoDoFo\n"
)
file(APPEND
    "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
    "set(PODOFO_INCLUDES ${PoDoFo_SOURCE_DIR}/src)\n"
)

add_subdirectory(src/podofo)
if(NOT PODOFO_BUILD_LIB_ONLY)
    # Tell gtest to use shared crt (option /MD)
    set(gtest_force_shared_crt ON)
    enable_testing()
    add_subdirectory(extern/gtest)
    add_subdirectory(test)
    add_subdirectory(examples)
endif(NOT PODOFO_BUILD_LIB_ONLY)

# Generate our configure file
configure_file(${PoDoFo_SOURCE_DIR}/podofo_config.h.in ${PoDoFo_BINARY_DIR}/podofo_config.h)

# Export some variables into the config file so it's easier for others
# to build and link against PoDoFo

# To use these dependencies set PODOFO_DIR to the podofo BUILD directory in
# your build (eg -DPODOFO_DIR=/path/to/podofo when running cmake to configure
# the app that'll use podofo). See: find_package(...) in the cmake docs.
if(PODOFO_BUILD_SHARED)
    export(TARGETS podofo_shared FILE "${CMAKE_CURRENT_BINARY_DIR}/PoDoFoConfig.cmake")
endif(PODOFO_BUILD_SHARED)
if(PODOFO_BUILD_STATIC)
    export(TARGETS podofo_static FILE "${CMAKE_CURRENT_BINARY_DIR}/PoDoFoConfig.cmake")
endif(PODOFO_BUILD_STATIC)
