/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfMemoryManagement.h"
#include "PdfDefines.h"

#ifndef SIZE_MAX
#define SIZE_MAX std::numeric_limits<size_t>::max()
#endif

#include <cerrno>

namespace mm {

void* pdfmm_calloc(size_t nmemb, size_t size)
{
	/*
		calloc behaviour with size==0 or nmemb==0 is platform specific

		Windows Visual C++
		Behaviour unspecified in MSDN (allocates 1 byte in VC 2015 calloc_base.c)

		OpenBSD
		If size or nmemb is equal to 0, a unique pointer to an access protected, zero sized object is returned. 
        Access via this pointer will generate a SIGSEGV exception.

		Linux
		If size or nmemb is equal to 0 then calloc() returns either nullptr, or a unique pointer value that can later be successfully passed to free().

		OS X
		Behaviour unspecified
	*/

	if (size == 0)
		size = 1;
    
	if (nmemb == 0)
		nmemb = 1;

	/*
		This overflow check is from OpenBSD reallocarray.c, and is also used in GifLib 5.1.2 onwards.
		
        Very old versions of calloc() in NetBSD and OS X 10.4 just multiplied size*nmemb which can
        overflow size_t and allocate much less memory than expected e.g. 2*(SIZE_MAX/2+1) = 2 bytes. 
        The calloc() overflow is also present in GCC 3.1.1, GNU Libc 2.2.5 and Visual C++ 6.
        http://cert.uni-stuttgart.de/ticker/advisories/calloc.html

		MUL_NO_OVERFLOW is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
		if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
	*/
	#define MUL_NO_OVERFLOW	((size_t)1 << (sizeof(size_t) * 4))
    
	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
		nmemb > 0 && SIZE_MAX / nmemb < size) 
	{
		errno = ENOMEM;
		return nullptr;
	}

	return calloc(nmemb, size);
}

void* pdfmm_realloc( void* buffer, size_t size )
{
	/*
		realloc behaviour with size==0 is platform specific (and dangerous in Visual C++)
	
		Windows Visual C++
		If size is zero, then the block pointed to by memblock is freed; the return value is nullptr, and buffer is left pointing at a freed block.
		NOTE: this is very dangerous, since nullptr is also returned when there's not enough memory, but the block ISN'T freed

		OpenBSD
		If size is equal to 0, a unique pointer to an access protected, zero sized object is returned. 
        Access via this pointer will generate a SIGSEGV exception.

		Linux
		If size was equal to 0, either nullptr or a pointer suitable to be passed to free() is returned.

		OS X
		If size is zero and buffer is not nullptr, a new, minimum sized object is allocated and the original object is freed.	
	*/

	if (size == 0)
		size = 1;

    return realloc( buffer, size );
}

void pdfmm_free( void* buffer )
{
    free( buffer );
}

};

