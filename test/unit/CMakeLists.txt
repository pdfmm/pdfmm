file(GLOB SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h" "*.cpp")
source_group("" FILES ${SOURCE_FILES})

include_directories(
    ${Fontconfig_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

# repeat for each test
add_executable(pdfmm-unit ${SOURCE_FILES})
target_link_libraries(pdfmm-unit ${PDFMM_LIBRARIES} ${Fontconfig_LIBRARIES} ${FREETYPE_LIBRARIES})
add_compile_options(${PDFMM_CFLAGS})

include(CTest)
include(Catch)
catch_discover_tests(pdfmm-unit)
