file(GLOB SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h" "*.cpp")
source_group("" FILES ${SOURCE_FILES})

# repeat for each test
add_executable(pdfmm-test ${SOURCE_FILES})

add_compile_options(${PDFMM_CFLAGS})
target_link_libraries(pdfmm-test ${PDFMM_LIBRARIES} gtest_main)
add_test(pdfmm-test pdfmm-test)
