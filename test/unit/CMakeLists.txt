file(GLOB SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h" "*.cpp")
source_group("" FILES ${SOURCE_FILES})

file(TO_NATIVE_PATH "${PDF_TEST_RESOURCE_PATH}" PDF_TEST_RESOURCE_PATH)
file(TO_NATIVE_PATH "${PDF_TEST_OUTPUT_PATH}" PDF_TEST_OUTPUT_PATH)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TestConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/TestConfig.h)

include_directories(
    ${Fontconfig_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# repeat for each test
add_executable(pdfmm-unit ${SOURCE_FILES})
target_link_libraries(pdfmm-unit
    ${PDFMM_LIBRARIES}
    pdfmm_private
    ${PDFMM_LIB_DEPENDS}
)
add_compile_options(${PDFMM_CFLAGS})

include(CTest)
include(Catch)
catch_discover_tests(pdfmm-unit)
