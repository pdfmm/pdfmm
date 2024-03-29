
# Playground

In this folder you can find some ready to use bootstrap scripts.
You must download deps first by running the following command:

    git submodule update --init

Then pick the appropriate script for your platform:

 - linux x86_64: `bootstrap-linux.sh`;
 - macos x86_64: `bootstrap-macos.sh`;
 - Windows x64: `bootstrap-win.cmd`;

Run it from the command line and then either:

 - Run the compilation with `cmake --build build-<arch>-<config>`, for example `cmake --build build-linux-release`;
 - *(Unix)* Run the compilation by entering the build directory and launch the compilation with `make`, for example `cd build-linux-debug && make`;
 - (Windows) Open the visual studio solution, for example `build-win-Debug\pdfmm.sln`.

## Testing

You can run the tests in the library by entering the
build directory and using [`ctest`](https://cmake.org/cmake/help/latest/manual/ctest.1.html),
for example `cd build-linux-debug && ctest`.
