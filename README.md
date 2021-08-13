# pdfmm

1.  [What is pdfmm?](#what_is_pdfmm)
2.  [Requirements](#requirements)
3.  [String encoding](#string_encoding_conventions)
4.  [API Stability](#api_stability)
5.  [TODO](#todo)
6.  [Licensing](#licensing)
7.  [No warranty](#no_warranty)
8.  [Contributions](#contributions)
9.  [Authors](#authors)

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

Short version: LGPL 2.1.

Long version: while several source code files are still
released under the old PoDoFo terms (LGPL 2.0 license or later),
new files are expected to be licensed under the terms
of the LGPL 2.1 license, so as a whole the library is
licensed under this specific license version.
See source headers for details. The tests and examples which are included in pdfmm are
licensed under the GPL 2.0. See all the COPYING files in the relevant
folders for details.

## String encoding conventions

All ```std::strings``` or ```std::string_view``` in the library are inteded
to hold UTF-8 encoded string content unless the documentation states otherwise.
For example ```std::string_view``` class can also represents a generic ```char``` buffer.
```PdfString``` and ```PdfName``` constructors accept UTF-8 encoded strings by default
(```PdfName``` accept only characters in the ```PdfDocEncoding``` char set, though).

## API stability

pdfmm has an unstable API that is the results of an extensive API review of PoDoFo.
It may converge to a stable API as soon as the review process is completed.
See [API Stability](https://github.com/pdfmm/pdfmm/wiki/API-Stability) for more details.

## TODO

There's a TODO [list](https://github.com/pdfmm/pdfmm/wiki/TODO) and a list of
planned [tasks](https://github.com/pdfmm/pdfmm/issues?q=is%3Aissue+is%3Aopen+label%3Aup-for-grabs)
in the issue tracker.

## No warranty

pdfmm may or may not work for your needs and comes with absolutely no warranty.
Serious bugs, including security flaws, may be fixed at arbitrary
timeframes, or not fixed at all.

## Contributions

If you find a bug and know how to fix it, or you want to add a small feature, you're welcome to send a [pull request](https://github.com/pdfmm/pdfmm/pulls),
providing it follows the [coding style](https://github.com/pdfmm/pdfmm/blob/master/CODINGSTYLE.md)
of the project. Also, as a minimum requisite, any contribution should be valuable for several people,
not only for a very restricted use case. For example, if the contributor wants to
add access to some currently unhandled PDF properties within an object, the contribution
should cover most of those or the most important ones, not only a single one. This
should avoids the contibutions to be only self relevant.
If you need to implement a bigger feature or refactor, check first if
it was already [planned](https://github.com/pdfmm/pdfmm/issues?q=is%3Aissue+is%3Aopen+label%3Aplanned-task)
in the issue list. The feature may be up for grabs, meaning that it's open for external contributions.
Please write in the relevant issue that you started to work on that, to receive some coordination.
If it's not, it means that the refactor/feature is planned to be implemented later by the maintainer(s).
If the feature is not listed in the issues, add it and/or create a [discussion](https://github.com/pdfmm/pdfmm/discussions)
to receive coordination and discuss some basic design choices.

## Authors

pdfmm is currently developed and mantained by
[Francesco Pretto ](mailto:ceztko@gmail.com).
pdfmm was forked from the PoDoFo library, which was
written by Dominik Seichter, Leonard Rosenthol,
Craig Ringer and others. See the file
AUTHORS for more details.