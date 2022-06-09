#ifndef PDFMM_VERSION_H
#define PDFMM_VERSION_H

// NOTE: Keep the relative import to accomodate
// the installation layout
#include "pdfmm_config.h"

/**
 * pdfmm version - 24-bit integer representation.
 * Version is 0xMMmmpp  where M is major, m is minor and p is patch
 * eg 0.7.0  is represented as 0x000700
 * eg 0.7.99 is represented as 0x000763
 *
 * Note that the pdfmm version is available in parts as individual 8-bit
 * integer literals in PDFMM_VERSION_MAJOR, PDFMM_VERSION_MINOR and
 * PDFMM_VERSION_PATCH .
 */
#define PDFMM_MAKE_VERSION_REAL(M,m,p) ( (M<<16)+(m<<8)+(p) )
#define PDFMM_MAKE_VERSION(M,m,p) PDFMM_MAKE_VERSION_REAL(M,m,p)
#define PDFMM_VERSION PDFMM_MAKE_VERSION(PDFMM_VERSION_MAJOR, PDFMM_VERSION_MINOR, PDFMM_VERSION_PATCH)

 /**
  * pdfmm version represented as a string literal, eg '0.7.99'
  */
  // The \0 is from Win32 example resources and the other values in pdfmm's one
#define PDFMM_MAKE_VERSION_STR_REAL(M,m,p) M ## . ## m ## . ## p
#define PDFMM_STR(x) #x "\0"
#define PDFMM_XSTR(x) PDFMM_STR(x)
#define PDFMM_MAKE_VERSION_STR(M,m,p) PDFMM_XSTR(PDFMM_MAKE_VERSION_STR_REAL(M,m,p))
#define PDFMM_VERSION_STRING PDFMM_MAKE_VERSION_STR(PDFMM_VERSION_MAJOR, PDFMM_VERSION_MINOR, PDFMM_VERSION_PATCH)

#endif // PDFMM_VERSION_H
