# pdfmm

1.  [What is pdfmm?](#what_is_pdfmm)
2.  [Requirements](#requirements)
3.  [Licensing](#licensing)
4.  [Authors](#authors)

## What is pdfmm?

pdfmm is a library to work with the PDF file format.
pdfmm is a derivative work of the [PoDoFo](http://podofo.sourceforge.net/)
library, from which it forked at revision [Rev@1999](https://sourceforge.net/p/podofo/code/1999/).

The pdfmm library is a free portable C++ library which
includes classes to parse a PDF file and modify its contents
into memory. The changes can be written back to disk easily.
Besides PDF parsing pdfmm includes also very simple classes
to create your own PDF files. It currently does not include
facilities to render pdf contents.

## Requirements

To build pdfmm lib you'll need a c++17 compiler,
CMake 3.16 and the following libraries:

* zlib
* freetype2
* fontconfig (Unix platforms only)
* OpenSSL
* libjpeg (optional)
* libtiff (optional)
* libidn (optional)

pdfmm is currently tested with the following compilers:

* Visual Studio 2017 15.9
* Visual Studio 2019 16.10
* gcc 7.3.1
* clang 5.0

## Licensing

The library is licensed under the LGPL (you can
use the shared library in closed sourced applications).
The tests and examples which are included in pdfmm are
licensed under the GPL. See the COPYING files for
the details.

## Authors

pdfmm is currently develoved and mantained by
[Francesco Pretto ](mailto:ceztko@gmail.com).
pdfmm was forked from the PoDoFo library, which was
written by Dominik Seichter, Leonard Rosenthol,
Craig Ringer and others. See the file
AUTHORS for more details.