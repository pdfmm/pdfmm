/**
 * SPDX-FileCopyrightText: (C) 2005 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDFMM_DEFS_H
#define PDFMM_DEFS_H

/*
 * This header provides a macro to handle correct symbol imports/exports
 * on platforms that require explicit instructions to make symbols public,
 * or differentiate between exported and imported symbols.
 * 
 * Win32 compilers use this information, and gcc4 can use it on *nix
 * to reduce the size of the export symbol table and get faster runtime
 * linking.
 *
 * All declarations of public API should be marked with the PDFMM_API macro.
 * Separate definitions need not be annotated, even in headers.
 *
 * Usage examples:
 *
 * class PDFMM_API PdfArray : public PdfDataContainer {
 *     ...
 * };
 *
 * bool PDFMM_API doThatThing();
 *
 * For an exception type that may be thrown across a DSO boundary, you must
 * use:
 *
 * class PDFMM_EXCEPTION_API(PDFMM_API) MyException
 * {
 *     ...
 * };
 *
 */

// Automatically defined by CMake when building a shared library
#if defined(pdfmm_shared_EXPORTS)
    #define COMPILING_SHARED_PDFMM
#endif

// Sanity check - can't be both compiling and using shared pdfmm
#if defined(SHARED_PDFMM) && defined(STATIC_PDFMM)
    #error "Both SHARED_PDFMM and STATIC_PDFMM defined!"
#endif

// Define SHARED_PDFMM when building the pdfmm library as a
// DLL. When building code that uses that DLL, define STATIC_PDFMM.
//
// Building or linking to a static library does not require either
// preprocessor symbol.
#ifdef PDFMM_STATIC

#define PDFMM_API
#define PDFMM_EXPORT
#define PDFMM_IMPORT

#elif PDFMM_SHARED

#if defined(_MSC_VER)
    #define PDFMM_EXPORT __declspec(dllexport)
    #define PDFMM_IMPORT __declspec(dllimport)
#else
    // NOTE: In non MSVC compilers https://gcc.gnu.org/wiki/Visibility,
    // it's not necessary to distinct between exporting and importing
    // the symbols and for correct working of RTTI features is better
    // always set default visibility both when compiling and when using
    // the library. The symbol will not be re-exported by other libraries
    #define PDFMM_EXPORT __attribute__ ((visibility("default")))
    #define PDFMM_IMPORT __attribute__ ((visibility("default")))
#endif

#if defined(COMPILING_SHARED_PDFMM)
#define PDFMM_API PDFMM_EXPORT
#else
#define PDFMM_API PDFMM_IMPORT
#endif

#else
#error Neither STATIC_PDFMM or SHARED_PDFMM defined
#endif

// Throwable classes must always be exported by all binaries when
// using gcc. Marking exception classes with PDFMM_EXCEPTION_API
// ensures this.
#ifdef _WIN32
  #define PDFMM_EXCEPTION_API(api) api
#else
  #define PDFMM_EXCEPTION_API(api) PDFMM_API
#endif

// Set up some other compiler-specific but not platform-specific macros

#ifdef __GNU__
  #define PDFMM_HAS_GCC_ATTRIBUTE_DEPRECATED 1
#elif defined(__has_attribute)
  #if __has_attribute(__deprecated__)
    #define PDFMM_HAS_GCC_ATTRIBUTE_DEPRECATED 1
  #endif
#endif

#ifdef PDFMM_HAS_GCC_ATTRIBUTE_DEPRECATED
    // gcc (or compat. clang) will issue a warning if a function or variable so annotated is used
    #define PDFMM_DEPRECATED __attribute__((__deprecated__))
#else
    #define PDFMM_DEPRECATED
#endif

#ifndef PDFMM_UNIT_TEST
#define PDFMM_UNIT_TEST(classname)
#endif

// Disable warnings
#ifdef _MSC_VER
#pragma warning(disable: 4251)
#pragma warning(disable: 4309)
#endif // _WIN32

#endif // PDFMM_DEFS_H
