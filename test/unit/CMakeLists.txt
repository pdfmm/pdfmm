file(GLOB SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h" "*.cpp")
source_group("" FILES ${SOURCE_FILES})

include_directories(
    ${Fontconfig_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# repeat for each test
add_executable(pdfmm-unit ${SOURCE_FILES})
target_link_libraries(pdfmm-unit
    pdfmm_test
    ${PDFMM_LIBRARIES}
    pdfmm_private
    ${PDFMM_LIB_DEPENDS}
)
add_compile_options(${PDFMM_CFLAGS})

include(CTest)
include(Catch)
catch_discover_tests(pdfmm-unit)
