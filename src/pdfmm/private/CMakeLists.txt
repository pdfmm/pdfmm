file(GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")

# Set visual studio source group
source_group("" FILES ${SOURCE_FILES})

if(NOT PDFMM_HAVE_JPEG_LIB)
    # Remove Jpeg source if not needed
    list(REMOVE_ITEM SOURCE_FILES
        JpegCommon.h
        JpegCommon.cpp
    )
endif()

if (PDFMM_SHARED AND NOT MSVC)
    # Fixes "relocation against hidden symbol can not
    # be used when making a shared object"
    add_compile_options(-fPIC)
endif()

add_library(pdfmm_private STATIC ${SOURCE_FILES})
