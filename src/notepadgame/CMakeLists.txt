project(
    notepadgame
    DESCRIPTION ""
    VERSION 0.0.1)

# patch scintilla source code
set(NPGAME_SCI_PATCH_FILE
    "${CMAKE_SOURCE_DIR}/extern/patch/scintilla/src/Editor.cxx")
set(NPGAME_SCI_ORIG_FILE "${NPGAME_SCINTILLA_DIR}/src/Editor.cxx")
set(NPGAME_SCI_TEMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/Editor_temp.cxx")
set(NPGAME_SCI_OUT_FILE "${NPGAME_SCINTILLA_DIR}/bin/Scintilla.dll")
set(patch_command ${CMAKE_COMMAND} -E copy_if_different ${NPGAME_SCI_PATCH_FILE} "./src/Editor.cxx")
include(ExternalProject)
ExternalProject_Add(scintilla
    PREFIX ${CMAKE_SOURCE_DIR}/extern/scintilla
    URL "https://sourceforge.net/projects/scintilla/files/scintilla/5.3.7/scintilla537.zip/download"
    URL_HASH "SHA1=7a72a85442706acbcb4075816a5cc4e58a6bc21c"
    #PATCH_COMMAND ${patch_command}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cd win32 && nmake -f scintilla.mak
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE TRUE
        )



