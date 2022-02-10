# pdfmm [![build-linux](https://github.com/pdfmm/pdfmm/actions/workflows/build-linux.yml/badge.svg)](https://github.com/pdfmm/pdfmm/actions/workflows/build-linux.yml) [![build-mac](https://github.com/pdfmm/pdfmm/actions/workflows/build-mac.yml/badge.svg)](https://github.com/pdfmm/pdfmm/actions/workflows/build-mac.yml) [![build-win](https://github.com/pdfmm/pdfmm/actions/workflows/build-win.yml/badge.svg)](https://github.com/pdfmm/pdfmm/actions/workflows/build-win.yml)

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

pdfmm is a s a free portable C++ library to work with the PDF file format.
pdfmm is a derivative work of the [PoDoFo](http://podofo.sourceforge.net/)
library, from which it forked at [Rev@1999](https://sourceforge.net/p/podofo/code/1999/).

pdfmm provides classes to parse a PDF file and modify its content
into memory. The changes can be written back to disk easily.
Besides PDF parsing pdfmm also provides facilities to create your
own PDF files from scratch. It currently does not
support rendering PDF content.

## Requirements

To build pdfmm lib you'll need a c++17 compiler,
CMake 3.16 and the following libraries:

* freetype2
* fontconfig (required for Unix platforms, optional for Windows)
* OpenSSL
* LibXml2
* zlib
* libjpeg (optional)
* libtiff (optional)
* libpng (optional)
* libidn (optional)

For the most polular toolchains, pdfmm requires the following
minimum versions:

* Visual Studio 2017 15.9
* gcc 8.1
* clang/llvm 7.0

It is regularly tested with the following toolchain versions:

* Visual Studio 2017 15.9
* Visual Studio 2019 16.11
* gcc 9.3.1
* clang/llvm 11.0.0

## Licensing

Short version: LGPL 2.1.

Long version: while several source code files are still
released under the old PoDoFo terms (LGPL 2.0 license or later),
new files are expected to be licensed under the terms
of the LGPL 2.1 license, so as a whole the library is
licensed under this specific license version. See source headers
for details. The tests and examples which are included in pdfmm are
licensed under the GPL 2.0. This may change at a later stage.
See all the COPYING files in the relevant folders for details.

## Development quickstart

There's a playground area in the repository where you can find
have access to pre-build dependencies for some popular architectures/operating
systems. Have a look in the [Readme](https://github.com/pdfmm/pdfmm/tree/master/playground) there.
Also the github workflow [definition](https://github.com/pdfmm/pdfmm/tree/master/.github/workflows)
files are very useful as they provide booststrap commands to build
pdfmm with latest packages from [brew](https://brew.sh/) under mac and `apt-get` under ubuntu.

## String encoding and buffer conventions

All ```std::strings``` or ```std::string_view``` in the library are inteded
to hold UTF-8 encoded string content. ```PdfString``` and ```PdfName``` constructors
accept UTF-8 encoded strings by default (```PdfName``` accept only characters in the
```PdfDocEncoding``` char set, though). ```charbuff``` abd ```bufferview```
instead represent a generic octet buffer.

## API stability

pdfmm has an unstable API that is the results of an extensive API review of PoDoFo.
It may converge to a stable API as soon as the review process is completed.
See [API Stability](https://github.com/pdfmm/pdfmm/wiki/API-Stability) for more details.

## TODO

There's a [TODO](https://github.com/pdfmm/pdfmm/blob/master/TODO.md) list in the wiki and a list of
planned [tasks](https://github.com/pdfmm/pdfmm/issues?q=is%3Aissue+is%3Aopen+label%3Aup-for-grabs)
in the issue tracker. **Urgent help needed**: pdfmm is first unveiled with a serious lack,
which is the disabling of the unit test suite based on cppunit.
Another test suite not based on cppunit is run regularly but it can't be released,
since it has a fixture of real world PDFs that can't be released
without the permissions of the owners. See this [discussion](https://github.com/pdfmm/pdfmm/discussions/29)
for details and if you can provide some help.

## No warranty

pdfmm may or may not work for your needs and comes with absolutely no warranty.
Serious bugs, including security flaws, may be fixed at arbitrary
timeframes, or not fixed at all.

## Contributions

If you find a bug and know how to fix it, or you want to add a small feature, you're welcome to send a [pull request](https://github.com/pdfmm/pdfmm/pulls),
providing it follows the [coding style](https://github.com/pdfmm/pdfmm/blob/master/CODING-STYLE.md)
of the project. Also, as a minimum requisite, any contribution should be valuable for a multitude of people,
to avoid it to be only self relevant for the contributor. Other reasons for the rejection, or hold,
of a pull request may be:

* the change doesn't fit the scope of pdfmm;
* the change shows lack of knowledge/mastery of the PDF specification and/or C++ language;
* the change breaks automatic tests performed by the maintainer;
* general lack of time in reviewing and merging the change.

If you need to implement a bigger feature or refactor, check first if
it was already [planned](https://github.com/pdfmm/pdfmm/issues?q=is%3Aissue+is%3Aopen+label%3Afeatured-task)
in the issue list. The feature may be up for grabs, meaning that it's open for external contributions.
Please write in the relevant issue that you started to work on that, to receive some feedback/coordination.
If it's not, it means that the refactor/feature is planned to be implemented later by the maintainer(s).
If the feature is not listed in the issues, add it and/or create a [discussion](https://github.com/pdfmm/pdfmm/discussions)
to receive some feedback and discuss some basic design choices.

## Authors

pdfmm is currently developed and mantained by
[Francesco Pretto ](mailto:ceztko@gmail.com).
pdfmm was forked from the PoDoFo library, which was
written by Dominik Seichter, Leonard Rosenthol,
Craig Ringer and others. See the file
[AUTHORS.md](https://github.com/pdfmm/pdfmm/blob/master/AUTHORS.md) for more details.
