# This is not a full standalone CMake configuration for the hello world
# example.
#
# To build it outside the pdfmm source tree, you must set your build system
# make the pdfmm headers available and must link to the pdfmm library
# and any libraries it depends on (see the README,
#     "5. Using pdfmm in Your Application") .
#
# Note that you don't need the headers for pdfmm's dependencies in your
# header search path.

add_compile_options(${PDFMM_CFLAGS})
add_executable(helloworld-base14 helloworld-base14.cpp)
target_link_libraries(helloworld-base14 ${PDFMM_DEPEND_TARGET})
