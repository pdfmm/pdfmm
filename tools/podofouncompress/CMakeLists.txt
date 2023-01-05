add_executable(podofouncompress podofouncompress.cpp Uncompress.cpp Uncompress.h)
target_link_libraries(podofouncompress
	${PODOFO_LIBRARIES}
	pdfmm_private
)
install(TARGETS podofouncompress RUNTIME DESTINATION "bin")
