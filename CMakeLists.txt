CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#*****************  IMPORTANT  ************* IMPORTANT **********************
# Look at http://www.vtk.org/Wiki/CMake_HowToDoPlatformChecks
# and the other wiki entries before you add anything. You might not need to.
#****************************************************************************

#
# Project name and version
#
PROJECT(PoDoFo)

SET(PODOFO_VERSION_MAJOR "0" CACHE STRING "Major part of PoDoFo version number")
SET(PODOFO_VERSION_MINOR "9" CACHE STRING "Minor part of PoDoFo version number")
SET(PODOFO_VERSION_PATCH "6" CACHE STRING "Patchlevel part of PoDoFo version number")
SET(PODOFO_SOVERSION "${PODOFO_VERSION_MAJOR}.${PODOFO_VERSION_MINOR}.${PODOFO_VERSION_PATCH}")
SET(PODOFO_LIBVERSION "${PODOFO_SOVERSION}")


#
# Main includes
#
INCLUDE(CheckIncludeFile)
INCLUDE(CheckLibraryExists)
INCLUDE(TestBigEndian)
INCLUDE(CheckTypeSize)

set(CMAKE_CXX_STANDARD 17)

# Load modules from our source tree too
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")


# Builds must use this CMakeLists.txt, not the one in src/ or somewhere else.
# If users try to use something else the results can be confusing. We set a
# variable here that we require to be set elsewhere, otherwise we'll complain.
SET(PODOFO_MAIN_CMAKELISTS_READ TRUE)

# If the user hasn't told use specifically what they want, build only
# a static library or only a shared library on Windows.
IF(NOT DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
    IF(WIN32)
        SET(PODOFO_BUILD_STATIC FALSE)
        SET(PODOFO_BUILD_SHARED TRUE)
    ELSE(WIN32)
        SET(PODOFO_BUILD_STATIC TRUE)
        SET(PODOFO_BUILD_SHARED FALSE)
    ENDIF(WIN32)
ENDIF(NOT DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
IF(DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
    IF(PODOFO_BUILD_SHARED)
        SET(PODOFO_BUILD_STATIC FALSE)
    ELSE(PODOFO_BUILD_SHARED)
        SET(PODOFO_BUILD_STATIC TRUE)
    ENDIF(PODOFO_BUILD_SHARED)
ENDIF(DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
IF(NOT DEFINED PODOFO_BUILD_SHARED AND DEFINED PODOFO_BUILD_STATIC)
    IF(PODOFO_BUILD_STATIC)
        SET(PODOFO_BUILD_SHARED FALSE)
    ELSE(PODOFO_BUILD_STATIC)
        SET(PODOFO_BUILD_SHARED TRUE)
    ENDIF(PODOFO_BUILD_STATIC)
ENDIF(NOT DEFINED PODOFO_BUILD_SHARED AND DEFINED PODOFO_BUILD_STATIC)

IF(DEFINED LIB_SUFFIX)
	SET(LIBDIRNAME "lib${LIB_SUFFIX}")
ELSE(DEFINED LIB_SUFFIX)

# Some 64 bit linux distros use /usr/lib64 for 64 bit libraries.
# on these platforms we must
IF(NOT DEFINED WANT_LIB64)
	# TODO: detect 64-bit build and existance of /usr/lib64 and set to TRUE.
	MESSAGE("WANT_LIB64 unset; assuming normal library directory names")
	SET(WANT_LIB64 FALSE)
ENDIF(NOT DEFINED WANT_LIB64)

IF(WANT_LIB64)
	SET(LIBDIRNAME "lib64")
ELSE(WANT_LIB64)
	SET(LIBDIRNAME "lib")
ENDIF(WANT_LIB64)

ENDIF(DEFINED LIB_SUFFIX)
MESSAGE("Will install libraries to ${CMAKE_INSTALL_PREFIX}/${LIBDIRNAME}")

# Linux packagers want an uninstall target.
CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}"
    -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Check if we are big endian
TEST_BIG_ENDIAN(TEST_BIG)

CHECK_INCLUDE_FILE_CXX("optional" HAVE_STD_OPTIONAL)
if (NOT HAVE_STD_OPTIONAL)
    CHECK_INCLUDE_FILE_CXX("experimental/optional" HAVE_STD_EXPERIMENTAL_OPTIONAL)
            message("${HAVE_STD_EXPERIMENTAL_OPTIONAL}")
    if (NOT HAVE_STD_EXPERIMENTAL_OPTIONAL)
        message(FATAL_ERROR "std::optional is required")
    endif()
endif()

IF(WIN32)
    # We must explicitly link to the core win32 libraries
    SET(PLATFORM_SYSTEM_LIBRARIES kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid ws2_32)
    # Microsoft deprecate certain POSIX functions that we use.
    # for now, turn off these warnings.
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ELSE(WIN32)
    SET(PLATFORM_SYSTEM_LIBRARIES)
ENDIF(WIN32)

IF(UNIX AND NOT PODOFO_NO_FONTMANAGER)
    SET(WANT_FONTCONFIG TRUE CACHE INTERNAL
        "True if PoDoFo should be built with fontconfig support")
ELSE(UNIX AND NOT PODOFO_NO_FONTMANAGER)
    SET(WANT_FONTCONFIG FALSE CACHE INTERNAL
        "True if PoDoFo should be built with fontconfig support")
ENDIF(UNIX AND NOT PODOFO_NO_FONTMANAGER)


IF(CMAKE_COMPILER_IS_GNUCXX)
    MESSAGE("Using gcc specific compiler options")
    # We can be more specific about what we want out of g++
    # than with most other compilers.

    # If the user hasn't specifically said whether they want
    # -fvisibility=hidden or not, turn it on if it's said to
    # be supported, off for other gcc versions.
    IF(NOT DEFINED PODOFO_USE_VISIBILITY)
        SET(PODOFO_USE_VISIBILITY ${PODOFO_HAVE_GCC_SYMBOL_VISIBILITY})
    ENDIF(NOT DEFINED PODOFO_USE_VISIBILITY)

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Woverloaded-virtual -Wswitch -Wcast-qual -Wwrite-strings -Wredundant-decls -Wreorder -Wno-deprecated-declarations")

    #
    # Note that we do not need debug definitions here. Set
    # -DCMAKE_BUILD_TYPE=debug or (if you want an optimised
    # release build with debug info) -DCMAKE_CXX_FLAGS="-g3"
    #
    # We add -W unless we're using gcc on win32, where it produces
    # spurious warnings about dllimport of inlines because of a dllimport
    # declaration on the whole class.
    IF(NOT WIN32)
        ADD_DEFINITIONS(-W)
    ENDIF(NOT WIN32)
    # If they've enabled the use of gcc4 symbol visibility, use it.
    IF(PODOFO_USE_VISIBILITY)
        ADD_DEFINITIONS(
            -DPODOFO_HAVE_GCC_SYMBOL_VISIBILITY
            -fvisibility=hidden
            )
    ENDIF(PODOFO_USE_VISIBILITY)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

FIND_PACKAGE(ZLIB REQUIRED)
MESSAGE("Found zlib headers in ${ZLIB_INCLUDE_DIR}, library at ${ZLIB_LIBRARIES}")

FIND_PACKAGE(LIBCRYPTO)

IF(LIBCRYPTO_FOUND)
	SET(PODOFO_HAVE_OPENSSL TRUE)
	INCLUDE_DIRECTORIES(${LIBCRYPTO_INCLUDE_DIR})
	MESSAGE("Found OpenSSL's libCrypto headers in ${LIBCRYPTO_INCLUDE_DIR}, library at ${LIBCRYPTO_LIBRARIES}")
ELSE(LIBCRYPTO_FOUND)
	MESSAGE("OpenSSL's libCrypto not found. Encryption support will be disabled")
ENDIF(LIBCRYPTO_FOUND)

FIND_PACKAGE(LIBIDN)

IF(LIBIDN_FOUND)
  MESSAGE("Found libidn headers in ${LIBIDN_INCLUDE_DIR}, library at ${LIBIDN_LIBRARIES}")
ENDIF(LIBIDN_FOUND)

IF(LIBIDN_FOUND)
	SET(PODOFO_HAVE_LIBIDN TRUE)
	INCLUDE_DIRECTORIES(${LIBIDN_INCLUDE_DIR})
	MESSAGE("Libidn found. AES-256 Encryption support will be enabled")
ELSE(LIBIDN_FOUND)
	MESSAGE("Libidn not found. AES-256 Encryption support will be disabled")
ENDIF(LIBIDN_FOUND)

FIND_PACKAGE(LIBJPEG)

IF(LIBJPEG_FOUND)
  MESSAGE("Found libjpeg headers in ${LIBJPEG_INCLUDE_DIR}, library at ${LIBJPEG_LIBRARIES}")
  SET(PODOFO_HAVE_JPEG_LIB TRUE)
  INCLUDE_DIRECTORIES(${LIBJPEG_INCLUDE_DIR})
ELSE(LIBJPEG_FOUND)
  MESSAGE("Libjpeg not found. JPEG support will be disabled")
ENDIF(LIBJPEG_FOUND)

FIND_PACKAGE(TIFF)

IF(TIFF_FOUND)
  MESSAGE("Found libtiff headers in ${TIFF_INCLUDE_DIR}, library at ${TIFF_LIBRARIES}")
  SET(PODOFO_HAVE_TIFF_LIB TRUE)
  INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})
ELSE(TIFF_FOUND)
  MESSAGE("Libtiff not found. TIFF support will be disabled")
ENDIF(TIFF_FOUND)

FIND_PACKAGE(PNG)

IF(PNG_FOUND)
  MESSAGE("Found LibPng headers in ${PNG_INCLUDE_DIR}, library at ${PNG_LIBRARIES}")
  SET(PODOFO_HAVE_PNG_LIB TRUE)
  INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
ELSE(PNG_FOUND)
  MESSAGE("LibPng not found. PNG support will be disabled")
  SET(PNG_LIBRARIES "")
ENDIF(PNG_FOUND)

FIND_PACKAGE(UNISTRING)

IF(UNISTRING_FOUND)
  MESSAGE("Found LibUnistring headers in ${UNISTRING_INCLUDE_DIR}, library at ${UNISTRING_LIBRARY}")
  SET(PODOFO_HAVE_UNISTRING_LIB TRUE)
  INCLUDE_DIRECTORIES(${UNISTRING_INCLUDE_DIR})
ELSE(UNISTRING_FOUND)
  MESSAGE("LibUnistring not found. Unistring support will be disabled")
  SET(UNISTRING_LIBRARY "")
ENDIF(UNISTRING_FOUND)


IF(NOT PODOFO_BUILD_LIB_ONLY)

FIND_PACKAGE(CppUnit)

IF(CppUnit_FOUND)
  MESSAGE("Found cppunit. Unit tests will be built.")
  SET(PODOFO_HAVE_CPPUNIT CppUnit_FOUND)
  INCLUDE_DIRECTORIES(${CPPUNIT_INCLUDE_DIR})
ELSE(CppUnit_FOUND)
  MESSAGE("Cppunit not found. No unit tests will be built.")
ENDIF(CppUnit_FOUND)

ENDIF(NOT PODOFO_BUILD_LIB_ONLY)

FIND_PACKAGE(OpenSSL)

FIND_PACKAGE(Freetype REQUIRED)
MESSAGE("Found freetype library at ${FREETYPE_LIBRARIES}, headers ${FREETYPE_INCLUDE_DIRS}")

IF(WANT_FONTCONFIG)
	FIND_PACKAGE(Fontconfig REQUIRED)
	SET(PODOFO_HAVE_FONTCONFIG TRUE)
	SET(PODOFO_LIB_FONTCONFIG:STRING fontconfig)
	IF(FONTCONFIG_FOUND)
	  MESSAGE("Found fontconfig headers in ${FONTCONFIG_INCLUDE_DIR}, library at ${FONTCONFIG_LIBRARIES}")
	ELSE(FONTCONFIG_FOUND)
	  MESSAGE("Could not find fontconfig.")
        ENDIF(FONTCONFIG_FOUND)
ELSE(WANT_FONTCONFIG)
	# Might as well look for it anyway. This also sets the appropriate
	# variables to empty values.
	FIND_PACKAGE(Fontconfig)
	SET(PODOFO_LIB_FONTCONFIG:STRING)
ENDIF(WANT_FONTCONFIG)

IF(WANT_BOOST)
    MESSAGE("Looking optional for Boost.")
    MESSAGE("Boost is optional, so don't worry if it is not found.")
    MESSAGE("Set the BOOST_ROOT env var if you have problems.")
    FIND_PACKAGE(Boost)
    IF(BOOST_FOUND)
       SET(PODOFO_HAVE_BOOST TRUE)
       INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR})
    ELSE(BOOST_FOUND)
       MESSAGE("If you don't need graph support you can ignore the above error.")
    ENDIF(BOOST_FOUND)
ENDIF(WANT_BOOST)

INCLUDE_DIRECTORIES(BEFORE # before toolchain include dir (to ignore installed)
    ${PoDoFo_SOURCE_DIR}/src   # order will be reversed, so this is the second dir
    ${PoDoFo_BINARY_DIR}   # because of BEFORE, this is the first include dir
    )

INCLUDE_DIRECTORIES(
    ${PoDoFo_SOURCE_DIR}/src/podofo
	${PoDoFo_SOURCE_DIR}/3rdparty/include
    ${FREETYPE_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
     )
LINK_DIRECTORIES(
    ${PoDoFo_BINARY_DIR}/src/podofo
    )

#
# The PoDoFo library needs to be linked to these libraries,
# as do any apps or libraries linking to PoDoFo. PODOFO_LIB
# will include these and the correct podofo target, so clients
# should specify only PODOFO_LIB .
#
# The LIBCRYPTO_LDFLAGS is defined only if the libcrypto was found
# by pkg-config, otherwise it will be empty. It can contain proper
# path for the libcrypto too, thus it's added here.
#
SET(PODOFO_LIB_DEPENDS
  ${LIBIDN_LIBRARIES}
  ${LIBCRYPTO_LDFLAGS}
  ${LIBCRYPTO_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${LIBJPEG_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  ${PNG_LIBRARIES}
  ${TIFF_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${UNISTRING_LIBRARY}
  ${PLATFORM_SYSTEM_LIBRARIES}
  )

IF(FONTCONFIG_FOUND AND WANT_FONTCONFIG)
  SET(PODOFO_LIB_DEPENDS ${FONTCONFIG_LIBRARIES} ${PODOFO_LIB_DEPENDS})
  INCLUDE_DIRECTORIES(${FONTCONFIG_INCLUDE_DIR})
ENDIF(FONTCONFIG_FOUND AND WANT_FONTCONFIG)

IF(WIN32 OR PODOFO_BUILD_STATIC)
  SET(PODOFO_LIB
      podofo
      ${PODOFO_LIB_DEPENDS}
      )
ELSE(WIN32 OR PODOFO_BUILD_STATIC)
  SET(PODOFO_LIB podofo)
ENDIF(WIN32 OR PODOFO_BUILD_STATIC)

# Create the config file. It'll be appended to as the subdirs run though
# then dependency information will be written to it at the end of the
# build.
FILE(WRITE
     "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
     "# CMake module for PoDoFo\n"
     )
FILE(APPEND 
     "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
     "SET(PODOFO_INCLUDES ${PoDoFo_SOURCE_DIR}/src)\n"
     )

ADD_SUBDIRECTORY(src/podofo)
IF(NOT PODOFO_BUILD_LIB_ONLY)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(examples)
ENDIF(NOT PODOFO_BUILD_LIB_ONLY)

# Generate our configure file
CONFIGURE_FILE(${PoDoFo_SOURCE_DIR}/podofo_config.h.in ${PoDoFo_BINARY_DIR}/podofo_config.h)

# Export some variables into the config file so it's easier for others
# to build and link against PoDoFo

# To use these dependencies set PODOFO_DIR to the podofo BUILD directory in
# your build (eg -DPODOFO_DIR=/path/to/podofo when running cmake to configure
# the app that'll use podofo). See: FIND_PACKAGE(...) in the cmake docs.
IF(PODOFO_BUILD_SHARED)
  EXPORT(TARGETS podofo_shared FILE "${CMAKE_CURRENT_BINARY_DIR}/PoDoFoConfig.cmake")
ENDIF(PODOFO_BUILD_SHARED)
IF(PODOFO_BUILD_STATIC)
  EXPORT(TARGETS podofo_static FILE "${CMAKE_CURRENT_BINARY_DIR}/PoDoFoConfig.cmake")
ENDIF(PODOFO_BUILD_STATIC)

