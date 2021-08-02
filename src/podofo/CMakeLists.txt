if(NOT PODOFO_MAIN_CMAKELISTS_READ)
    message(FATAL_ERROR "Run cmake on the CMakeLists.txt in the project root, not the one in the 'src' directory. You will need to delete CMakeCache.txt from the current directory.")
endif(NOT PODOFO_MAIN_CMAKELISTS_READ)

# Needed for <date/date.h>
add_definitions(-DHAS_UNCAUGHT_EXCEPTIONS=1)

set(PODOFO_BASE_DEPEND_TARGET CACHE INTERNAL
    "Which PoDoFo library target to depend on when building tools and tests")
set(PODOFO_DEPEND_TARGET CACHE INTERNAL
    "Which PoDoFo library target to depend on when building tools and tests")

set(PODOFO_BASE_SOURCES
    base/PdfArray.cpp
    base/PdfCanvas.cpp
    base/PdfColor.cpp
    base/PdfContainerDataType.cpp
    base/PdfContentsTokenizer.cpp
    base/PdfPostScriptTokenizer.cpp
    base/PdfData.cpp
    base/PdfDataType.cpp
    base/PdfDate.cpp
    base/PdfDefinesPrivate.cpp
    base/PdfDictionary.cpp
    base/PdfEncodingShim.cpp
    base/PdfEncoding.cpp
    base/PdfEncodingFactory.cpp
    base/PdfEncodingMap.cpp
    base/PdfEncodingMapFactory.cpp
    base/PdfCMapEncoding.cpp
    base/PdfPredefinedEncoding.cpp
    base/PdfFontType1Encoding.cpp
    base/PdfDifferenceEncoding.cpp
    base/PdfIdentityEncoding.cpp
    base/PdfEncrypt.cpp
    base/PdfError.cpp
    base/PdfFileStream.cpp
    base/PdfFilter.cpp
    base/PdfFiltersPrivate.cpp
    base/PdfImmediateWriter.cpp
    base/PdfInputDevice.cpp
    base/PdfCanvasInputDevice.cpp
    base/PdfInputStream.cpp
    base/PdfLocale.cpp
    base/PdfMemStream.cpp
    base/PdfMemoryManagement.cpp
    base/PdfName.cpp
    base/PdfObject.cpp
    base/PdfObjectStreamParser.cpp
    base/PdfOutputDevice.cpp
    base/PdfOutputStream.cpp
    base/PdfParser.cpp
    base/PdfParserObject.cpp
    base/PdfRect.cpp
    base/PdfRefCountedBuffer.cpp
    base/PdfRefCountedInputDevice.cpp
    base/PdfReference.cpp
    base/PdfSigner.cpp
    base/PdfStream.cpp
    base/PdfString.cpp
    base/PdfTextState.cpp
    base/PdfTokenizer.cpp
    base/PdfVariant.cpp
    base/PdfVecObjects.cpp
    base/PdfWriter.cpp
    base/PdfXRef.cpp
    base/PdfXRefEntry.cpp
    base/PdfXRefStream.cpp
    base/PdfXRefStreamParserObject.cpp
    base/PdfCharCodeMap.cpp
)

set(PODOFO_DOC_SOURCES
    doc/PdfAcroForm.cpp
    doc/PdfAction.cpp
    doc/PdfAnnotation.cpp
    doc/PdfContents.cpp
    doc/PdfDestination.cpp
    doc/PdfDocument.cpp
    doc/PdfElement.cpp
    doc/PdfExtGState.cpp
    doc/PdfField.cpp
    doc/PdfTextBox.cpp
    doc/PdfButton.cpp
    doc/PdfCheckBox.cpp
    doc/PdfButton.cpp
    doc/PdfPushButton.cpp
    doc/PdfCheckBox.cpp
    doc/PdfRadioButton.cpp
    doc/PdfChoiceField.cpp
    doc/PdfComboBox.cpp
    doc/PdfListBox.cpp
    doc/PdfSignature.cpp
    doc/PdfFileSpec.cpp
    doc/PdfFont.cpp
    doc/PdfFontObject.cpp
    doc/PdfFontCIDTrueType.cpp
    doc/PdfFontCache.cpp
    doc/PdfFontFactory.cpp
    doc/PdfFontMetrics.cpp
    doc/PdfFontFactoryBase14Data.cpp
    doc/PdfFontMetricsBase14.cpp
    doc/PdfFontMetricsFreetype.cpp
    doc/PdfFontMetricsObject.cpp
    doc/PdfFontSimple.cpp
    doc/PdfFontTrueTypeSubset.cpp
    doc/PdfFontTrueType.cpp
    doc/PdfFontType1.cpp
    doc/PdfFontType3.cpp
    doc/PdfFontType1Base14.cpp
    doc/PdfFunction.cpp
    doc/PdfImage.cpp
    doc/PdfInfo.cpp
    doc/PdfMemDocument.cpp
    doc/PdfNamesTree.cpp
    doc/PdfOutlines.cpp
    doc/PdfPage.cpp
    doc/PdfPagesTree.cpp
    doc/PdfPagesTreeCache.cpp
    doc/PdfPainter.cpp
    doc/PdfShadingPattern.cpp
    doc/PdfStreamedDocument.cpp
    doc/PdfTilingPattern.cpp
    doc/PdfXObject.cpp
)

if(WIN32 AND PODOFO_BUILD_SHARED)
    # If we build for windows systems, we also include the resource file
    set(PODOFO_DOC_SOURCES ${PODOFO_DOC_SOURCES} doc/podofo-doc.rc)
endif()

set(PODOFO_HEADERS
    podofo-base.h
    podofo.h
)

set(PODOFO_BASE_HEADERS
    ${PoDoFo_BINARY_DIR}/podofo_config.h
    base/Pdf3rdPtyForwardDecl.h
    base/PdfArray.h
    base/PdfCanvas.h
    base/PdfColor.h
    base/PdfCompilerCompat.h
    base/PdfCompilerCompatPrivate.h
    base/PdfContentsTokenizer.h
    base/PdfPostScriptTokenizer.h
    base/PdfData.h
    base/PdfDataType.h
    base/PdfContainerDataType.h
    base/PdfDate.h
    base/PdfDefines.h
    base/PdfDefinesPrivate.h
    base/PdfDictionary.h
    base/PdfEncodingShim.h
    base/PdfEncoding.h
    base/PdfEncodingPrivate.h
    base/PdfEncodingFactory.h
    base/PdfEncodingMap.h
    base/PdfEncodingMapFactory.h
    base/PdfCMapEncoding.h
    base/PdfPredefinedEncoding.h
    base/PdfFontType1Encoding.h
    base/PdfDifferenceEncoding.h
    base/PdfIdentityEncoding.h
    base/PdfEncrypt.h
    base/PdfExtension.h
    base/PdfError.h
    base/PdfFileStream.h
    base/PdfFilter.h
    base/PdfFiltersPrivate.h
    base/PdfImmediateWriter.h
    base/PdfInputDevice.h
    base/PdfCanvasInputDevice.h
    base/PdfInputStream.h
    base/PdfLocale.h
    base/PdfMemStream.h
    base/PdfMemoryManagement.h
    base/PdfName.h
    base/PdfObject.h
    base/PdfObjectStreamParser.h
    base/PdfOutputDevice.h
    base/PdfOutputStream.h
    base/PdfParser.h
    base/PdfParserObject.h
    base/PdfRect.h
    base/PdfRefCountedBuffer.h
    base/PdfRefCountedInputDevice.h
    base/PdfReference.h
    base/PdfSigner.h
    base/PdfStream.h
    base/PdfString.h
    base/PdfTextState.h
    base/PdfTokenizer.h
    base/PdfVariant.h
    base/PdfVecObjects.h
    base/PdfVersion.h
    base/PdfWriter.h
    base/PdfXRef.h
    base/PdfXRefEntry.h
    base/PdfXRefStream.h
    base/PdfXRefStreamParserObject.h
    base/PdfCharCodeMap.h
    base/podofoapi.h
)

set(PODOFO_DOC_HEADERS
    doc/PdfAcroForm.h
    doc/PdfAction.h
    doc/PdfAnnotation.h
    doc/PdfContents.h
    doc/PdfDestination.h
    doc/PdfDocument.h
    doc/PdfElement.h
    doc/PdfExtGState.h
    doc/PdfField.h
    doc/PdfTextBox.h
    doc/PdfButton.h
    doc/PdfCheckBox.h
    doc/PdfButton.h
    doc/PdfPushButton.h
    doc/PdfCheckBox.h
    doc/PdfRadioButton.h
    doc/PdfChoiceField.h
    doc/PdfComboBox.h
    doc/PdfListBox.h
    doc/PdfSignature.h
    doc/PdfFileSpec.h
    doc/PdfFont.h
    doc/PdfFontObject.h
    doc/PdfFontCIDTrueType.h
    doc/PdfFontCache.h
    doc/PdfFontFactory.h
    doc/PdfFontFactoryBase14Data.h
    doc/PdfFontMetrics.h
    doc/PdfFontMetricsBase14.h
    doc/PdfFontMetricsFreetype.h
    doc/PdfFontMetricsObject.h
    doc/PdfFontSimple.h
    doc/PdfFontTrueTypeSubset.h
    doc/PdfFontTrueType.h
    doc/PdfFontType1.h
    doc/PdfFontType3.h
    doc/PdfFontType1Base14.h
    doc/PdfFunction.h
    doc/PdfImage.h
    doc/PdfInfo.h
    doc/PdfMemDocument.h
    doc/PdfNamesTree.h
    doc/PdfOutlines.h
    doc/PdfPage.h
    doc/PdfPagesTree.h
    doc/PdfPagesTreeCache.h
    doc/PdfPainter.h
    doc/PdfShadingPattern.h
    doc/PdfStreamedDocument.h
    doc/PdfTilingPattern.h
    doc/PdfXObject.h
)

if(PODOFO_HAVE_FONTCONFIG)
    list(APPEND PODOFO_DOC_HEADERS doc/PdfFontConfigWrapper.h)
    list(APPEND PODOFO_DOC_SOURCES doc/PdfFontConfigWrapper.cpp)
endif()

set(PODOFO_HEADER_FILES
    ${PODOFO_HEADERS}
    ${PODOFO_BASE_HEADERS}
    ${PODOFO_DOC_HEADERS}
)

# Create a Source Groups for Visual Studio
source_group("" FILES ${PODOFO_HEADERS})
source_group("base" FILES ${PODOFO_BASE_HEADERS} ${PODOFO_BASE_SOURCES})
source_group("doc" FILES ${PODOFO_DOC_HEADERS} ${PODOFO_DOC_SOURCES})

install(FILES ${PODOFO_HEADERS} DESTINATION "include/podofo")
install(FILES ${PODOFO_BASE_HEADERS} DESTINATION "include/podofo/base")
install(FILES ${PODOFO_BASE_HEADERS2} DESTINATION "include/podofo/base/util")
install(FILES ${PODOFO_DOC_HEADERS} DESTINATION "include/podofo/doc")

if(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)
    message(FATAL_ERROR "At least one of PODOFO_BUILD_SHARED and PODOF_BUILD_STATIC must be set")
endif(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)

if(PODOFO_BUILD_STATIC)
    message("Building static PoDoFo library")
    add_library(podofo_static STATIC ${PODOFO_BASE_SOURCES} ${PODOFO_DOC_SOURCES} ${PODOFO_HEADER_FILES})
    target_link_libraries(podofo_static ${PODOFO_LIB_DEPENDS})
    set_target_properties(podofo_static PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
        COMPILE_FLAGS "-DBUILDING_PODOFO"
    )
    set(PODOFO_DEPEND_TARGET podofo_static
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    set(USING_SHARED_PODOFO FALSE)
    install(TARGETS podofo_static
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "${LIBDIRNAME}"
        ARCHIVE DESTINATION "${LIBDIRNAME}"
    )
endif(PODOFO_BUILD_STATIC)

if(PODOFO_BUILD_SHARED)
    message("Building shared PoDoFo library")
    add_library(podofo_shared SHARED ${PODOFO_BASE_SOURCES} ${PODOFO_DOC_SOURCES} ${PODOFO_HEADER_FILES})
    target_link_libraries(podofo_shared ${PODOFO_LIB_DEPENDS})
    # TODO: set /wd4251 flag if we're doing a debug build with
    # Visual Studio, since it produces invalid warnings about STL
    # use.
    set_target_properties(podofo_shared PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
        COMPILE_FLAGS "-DBUILDING_PODOFO"
    )
    # Since we're building a shared podofo, prefer to depend on this one for
    # tests and tools over the static library (if built).
    set(PODOFO_DEPEND_TARGET podofo_shared
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    set(USING_SHARED_PODOFO TRUE)
    install(TARGETS podofo_shared
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "${LIBDIRNAME}"
        ARCHIVE DESTINATION "${LIBDIRNAME}"
    )

    # Create a pkg-config file for linking against shared library
    # if pkg-config is available on the system.
    # Add a version to the file name corresponding to the API compatibility.

    find_program(PKG_CONFIG_FOUND pkg-config)
    if(PKG_CONFIG_FOUND)
        message("Pkg-config found, creating a pkg-config file for linking against shared library.")
        configure_file(
            "libpodofo.pc.in"
            "${PoDoFo_BINARY_DIR}/libpodofo.pc"
            @ONLY)
        install(
            FILES "${PoDoFo_BINARY_DIR}/libpodofo.pc"
            DESTINATION "${LIBDIRNAME}/pkgconfig")
    else(PKG_CONFIG_FOUND)
        message("Pkg-config not found. No pkg-config file will be created.")
    endif(PKG_CONFIG_FOUND)
endif(PODOFO_BUILD_SHARED)

# Use these flags when compiling code that includes PoDoFo headers.
# Failure to do so will result in compilation or link-time errors
# on some platforms, and can even cause undefined results at runtime.
if(WIN32 AND USING_SHARED_PODOFO)
    set(PODOFO_CFLAGS "-DUSING_SHARED_PODOFO" CACHE INTERNAL "Extra flags required when linking to PoDoFo")
else(WIN32 AND USING_SHARED_PODOFO)
    set(PODOFO_CFLAGS "" CACHE INTERNAL "Extra flags required when linking to PoDoFo")
endif(WIN32 AND USING_SHARED_PODOFO)

# Write the cflags and depend target to the config file
file(APPEND
    "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
    "set(PODOFO_CFLAGS ${PODOFO_CFLAGS})\n"
)
file(APPEND
    "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
    "set(PODOFO_DEPEND_TARGET ${PODOFO_DEPEND_TARGET})\n"
)
