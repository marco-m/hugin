# Application name
set( PROGNAME maskeditor )



IF(wxWidgets_FOUND)
  IF(WIN32)
    IF ("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
      ADD_DEFINITIONS("-D_UNICODE")
    ENDIF("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
  ENDIF(WIN32)

	# sources
	set(maskeditor_SOURCES
	gui/MaskEdApp.cpp gui/MaskEdMainFrame.cpp 
	gui/MaskEdClientWnd.cpp gui/MaskEdEditWnd.cpp gui/MaskEdPreviewWnd.cpp
	core/MaskMgr.cpp)
	
	IF (WIN32)
	  ADD_EXECUTABLE( ${PROGNAME} WIN32 ${maskeditor_SOURCES})
	ELSE (WIN32)
	  ADD_EXECUTABLE( ${PROGNAME} ${maskeditor_SOURCES} )
	ENDIF(WIN32)

	target_link_libraries( ${PROGNAME} ${common_libs} ${image_libs} huginbasewx ${wxWidgets_LIBRARIES})
	install(TARGETS ${PROGNAME} DESTINATION ${BINDIR})

  add_subdirectory(gui)
  #add_subdirectory(maskmgr)
ENDIF(wxWidgets_FOUND)

