# may break for APPLE...sorry :(
# Application name
set( PROGNAME maskeditor )

# sources
set(maskeditor_SOURCES
MaskEdApp.cpp MaskEdMainFrame.cpp)

IF (WIN32)
  ADD_EXECUTABLE( ${PROGNAME} WIN32 ${maskeditor_SOURCES})
ELSE (WIN32)
  ADD_EXECUTABLE( ${PROGNAME} ${maskeditor_SOURCES} )
ENDIF(WIN32)

target_link_libraries( ${PROGNAME} ${common_libs} ${image_libs} huginbasewx ${wxWidgets_LIBRARIES})
install(TARGETS ${PROGNAME} DESTINATION ${BINDIR})

add_subdirectory(xrc)

