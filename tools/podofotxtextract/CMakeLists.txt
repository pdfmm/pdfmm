add_executable(podofotxtextract podofotxtextract.cpp)
target_link_libraries(podofotxtextract
    ${PODOFO_LIBRARIES}
    pdfmm_private
)
install(TARGETS podofotxtextract RUNTIME DESTINATION "bin")
