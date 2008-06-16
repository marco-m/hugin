# Application name
set( PROGNAME maskeditor )

include_directories( ${CMAKE_SOURCE_DIR}/src/hugin1 )

IF(wxWidgets_FOUND)
  IF(WIN32)
    IF ("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
      ADD_DEFINITIONS("-D_UNICODE")
    ENDIF("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
  ENDIF(WIN32)

	# sources
	set(maskeditor_SOURCES
				gui/MaskEdApp.h gui/MaskEdApp.cpp
				gui/MaskEdMainFrame.h gui/MaskEdMainFrame.cpp 
				gui/MaskEdClientWnd.h gui/MaskEdClientWnd.cpp 
				gui/MaskEdEditWnd.h gui/MaskEdEditWnd.cpp 
				gui/MaskEdPreviewWnd.h gui/MaskEdPreviewWnd.cpp
				core/BrushStroke.h
				core/MaskPoly.h
				core/MaskMgr.h core/MaskMgr.cpp
				core/MaskFileMgr.h core/MaskFileMgr.cpp
				core/PTOFileMgr.h core/PTOFileMgr.cpp 
				core/MaskEdCommand.h core/MaskEdCommand.cpp)
	
	IF (WIN32)
	  ADD_EXECUTABLE( ${PROGNAME} WIN32 ${maskeditor_SOURCES})
	ELSE (WIN32)
	  ADD_EXECUTABLE( ${PROGNAME} ${maskeditor_SOURCES} )
	ENDIF(WIN32)

	target_link_libraries( ${PROGNAME} ${common_libs} ${image_libs} huginbasewx ${wxWidgets_LIBRARIES})
	install(TARGETS ${PROGNAME} DESTINATION ${BINDIR})

  add_subdirectory(gui)
  
ENDIF(wxWidgets_FOUND)

