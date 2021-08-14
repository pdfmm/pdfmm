## Source layout

### Root directory

- `3rdparty`: 3rd party headers and sources needed to compile pdfmm;
- `cmake`: [CMake](https://cmake.org/) scripts and includes needed to compile pdfmm;
- `examples`: Samples that use the pdfmm API;
- `extern`: external git submodules needed to compile pdfmm (no one is currently needed); 
- `src`: the main source directory;
- `test`: test suite.

### Source directory
The `src` directory contains only a single `pdfmm` folder.
In this way it's easy to just include `src` when using pdfmm
externally of the source tree, simulating the layout of prefixed
`include` after installation. The content of `pdfmm` is:

- `base`: base source directory with most of the library classes;
- `compat`: directory with compatibility headers needed to use/compile pdfmm;
- `contrib`: directory with more pdfmm API classes supplied by external contributors;
- `private`: directory with private headers/data needed to compile pdfmm.
