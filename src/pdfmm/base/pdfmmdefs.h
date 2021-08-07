/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDFMM_API_H_20061017
#define PDFMM_API_H_20061017

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
 * To hide a given method in an otherwise exported class:
 *
 * class PDFMM_API Myclass
 * {
 *     // blah blah
 * private:
 *     void privateHelper();
 * };
 *
 * For information on the gcc visibility support see:
 *     http://gcc.gnu.org/wiki/Visibility
 *     http://people.redhat.com/drepper/dsohowto.pdf
 *
 *
 *
 *
 * Note that gcc has some other useful attributes:
 *     http://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/Function-Attributes.html
 *     http://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/Variable-Attributes.html
 *     http://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/Type-Attributes.html
 *
 * including __attribute__((deprecated)) for deprecating old interfaces.
 *           (available as PDFMM_DEPRECATED)
 *
 *           __attribute__((pure)) for functions w/o side effects
 *           (available as PDFMM_PURE_FUNCTION)
 *
 */

// Peter Petrov 26 April 2008
/* Automatically defined by CMake when building a shared library */
#if defined (pdfmm_EXPORTS)
    #define COMPILING_SHARED_PDFMM
    #undef USING_SHARED_PDFMM
#endif

/* Automatically defined by CMake when building a shared library */
#if defined(pdfmm_shared_EXPORTS)
    #define COMPILING_SHARED_PDFMM
    #undef USING_SHARED_PDFMM
#endif

/* Sanity check - can't be both compiling and using shared pdfmm */
#if defined(COMPILING_SHARED_PDFMM) && defined(USING_SHARED_PDFMM)
    #error "Both COMPILING_SHARED_PDFMM and USING_SHARED_PDFMM defined!"
#endif

/*
 * Define COMPILING_SHARED_PDFMM when building the pdfmm library as a
 * DLL. When building code that uses that DLL, define USING_SHARED_PDFMM.
 *
 * Building or linking to a static library does not require either
 * preprocessor symbol.
 */
#if defined(WIN32)
    #if defined(COMPILING_SHARED_PDFMM)
        #define PDFMM_API __declspec(dllexport)
	#elif defined(USING_SHARED_PDFMM)
		#define PDFMM_API __declspec(dllimport)
    #else
        #define PDFMM_API
    #endif
#else
    #if defined(PDFMM_HAVE_GCC_SYMBOL_VISIBILITY)
        /* Forces inclusion of a symbol in the symbol table, so
           software outside the current library can use it. */
        #define PDFMM_API __attribute__ ((visibility("default")))
        /* Within a section exported with PDFMM_API, forces a symbol to be
           private to the library / app. Good for private members. */
        #define PDFMM_INTERNAL __attribute__ ((visibility("internal")))
    #else
        #define PDFMM_API
        #define PDFMM_INTERNAL
    #endif
#endif

/* Throwable classes must always be exported by all binaries when
 * using gcc. Marking exception classes with PDFMM_EXCEPTION_API
 * ensures this. */
#ifdef WIN32
  #define PDFMM_EXCEPTION_API(api) api
#elif defined(PDFMM_HAVE_GCC_SYMBOL_VISIBILITY)
  #define PDFMM_EXCEPTION_API(api) PDFMM_API
#else
  #define PDFMM_EXCEPTION_API(api)
#endif

/* Set up some other compiler-specific but not platform-specific macros */

#ifdef __GNU__
  #define PDFMM_HAS_GCC_ATTRIBUTE_DEPRECATED 1
#elif defined(__has_attribute)
  #if __has_attribute(__deprecated__)
    #define PDFMM_HAS_GCC_ATTRIBUTE_DEPRECATED 1
  #endif
#endif

#ifdef PDFMM_HAS_GCC_ATTRIBUTE_DEPRECATED
    /* gcc (or compat. clang) will issue a warning if a function or variable so annotated is used */
    #define PDFMM_DEPRECATED       __attribute__((__deprecated__))
#else
    #define PDFMM_DEPRECATED
#endif

// Peter Petrov 27 April 2008
// Disable warnings
#if defined(WIN32) && defined(_MSC_VER)
#pragma warning(disable: 4251)
#pragma warning(disable: 4309)
#endif // WIN32

#endif // PDFMM_API_H
