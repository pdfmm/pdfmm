cmake_minimum_required(VERSION 3.16)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Clean CMAKE_CONFIGURATION_TYPES. Only one configuration will be used
unset(CMAKE_CONFIGURATION_TYPES CACHE) # It will also remove it from GUI
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}")

project(pdfmm)
enable_testing() # Needed to enable "make test" from binary dir

if ("${PDFMM_DIR}" STREQUAL "")
    message("ARCH is not set. Run the boostrap scripts")
endif()

if ("${PDFMM_DIR}" STREQUAL "")
    get_filename_component(PDFMM_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
endif()

set(DEPS_PATH "$ENV{PDFMM_DEPS_PATH}")
if ("${DEPS_PATH}" STREQUAL "")
    set(DEPS_PATH "${PDFMM_DIR}/extern/deps/3rdparty")
endif()

if(NOT EXISTS "${DEPS_PATH}")
    message("Deps path doesn't exists: try fetch git submodules")
endif()

if (MSVC)
    # Ignore warnings about missing pdbs
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4099")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4099")
endif()

message("ARCHITECTURE: ${ARCH}")
message("PDFMM_DIR: ${PDFMM_DIR}")
message("DEPS_PATH: ${DEPS_PATH}")

# Set include folders for find_package()
list(APPEND CMAKE_INCLUDE_PATH
    "${DEPS_PATH}/zlib/include"
    "${DEPS_PATH}/libxml2/include"
    "${DEPS_PATH}/fontconfig/include"
    "${DEPS_PATH}/freetype/include"
    "${DEPS_PATH}/libpng/include"
    "${DEPS_PATH}/libjpeg/include"
    "${DEPS_PATH}/libtiff/include"
    "${DEPS_PATH}/openssl/include"
)

# Set library folders for find_package()
list(APPEND CMAKE_LIBRARY_PATH
    "${DEPS_PATH}/zlib/lib/${ARCH}"
    "${DEPS_PATH}/libxml2/lib/${ARCH}"
    "${DEPS_PATH}/freetype/lib/${ARCH}"
    "${DEPS_PATH}/fontconfig/lib/${ARCH}"
    "${DEPS_PATH}/libpng/lib/${ARCH}"
    "${DEPS_PATH}/libjpeg/lib/${ARCH}"
    "${DEPS_PATH}/libtiff/lib/${ARCH}"
    "${DEPS_PATH}/openssl/lib/${ARCH}"
)

# Set use libxml2 static for fontconfig
add_compile_definitions(LIBXML_STATIC)

if(WIN32)
    # Manually specify SSL
    set(OPENSSL_INCLUDE_DIR "${DEPS_PATH}/openssl/include" CACHE PATH "" FORCE)
    set(LIB_EAY_DEBUG "${DEPS_PATH}/openssl/lib/${ARCH}/cryptod.lib;Crypt32.lib" CACHE FILEPATH "" FORCE)
    set(LIB_EAY_RELEASE "${DEPS_PATH}/openssl/lib/${ARCH}/crypto.lib;Crypt32.lib" CACHE FILEPATH "" FORCE)
    set(SSL_EAY_DEBUG "${DEPS_PATH}/openssl/lib/${ARCH}/ssld.lib" CACHE FILEPATH "" FORCE)
    set(SSL_EAY_RELEASE "${DEPS_PATH}/openssl/lib/${ARCH}/ssl.lib" CACHE FILEPATH "" FORCE)
    # Manually specify Libidn
    # NOTE: Disabled for now
    #add_definitions(-DLIBIDN_STATIC)
    #set(LIBIDN_INCLUDE_DIR "${DEPS_PATH}/Libidn/include" CACHE PATH "" FORCE)
    #set(LIBIDN_LIBRARY_DEBUG "${DEPS_PATH}/Libidn/lib/${ARCH}/libidn.lib" CACHE FILEPATH "" FORCE)
    #set(LIBIDN_LIBRARY_RELEASE "${DEPS_PATH}/Libidn/lib/${ARCH}/libidn.lib" CACHE FILEPATH "" FORCE)
    # Libxml2
    set(LIBXML2_LIBRARY "optimized;${DEPS_PATH}/libxml2/lib/${ARCH}/libxml2.lib;debug;${DEPS_PATH}/libxml2/lib/${ARCH}/libxml2d.lib" CACHE FILEPATH "" FORCE)
    # Fontconfig
    set(Fontconfig_LIBRARY "optimized;${DEPS_PATH}/fontconfig/lib/${ARCH}/fontconfig.lib;debug;${DEPS_PATH}/fontconfig/lib/${ARCH}/fontconfigd.lib" CACHE FILEPATH "" FORCE)
else()
    # Freetype
    set(FREETYPE_INCLUDE_DIR_ft2build "${DEPS_PATH}/freetype/include/freetype2" CACHE PATH "" FORCE)
    set(FREETYPE_INCLUDE_DIR_freetype2 "${DEPS_PATH}/freetype/include" CACHE PATH "" FORCE)
    set(FREETYPE_LIBRARY_RELEASE "${DEPS_PATH}/freetype/lib/${ARCH}/libfreetype.a;${DEPS_PATH}/bzip2/lib/${ARCH}/libbz2.a" CACHE FILEPATH "" FORCE)
    # Fontconfig
    set(Fontconfig_LIBRARY "${DEPS_PATH}/fontconfig/lib/${ARCH}/libfontconfig.a;${DEPS_PATH}/libuuid/lib/${ARCH}/libuuid.a" CACHE FILEPATH "" FORCE)
endif()

add_subdirectory("${PDFMM_DIR}" "${CMAKE_BINARY_DIR}/pdfmm")
