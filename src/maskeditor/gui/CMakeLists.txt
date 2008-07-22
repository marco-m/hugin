# Application name
set( PROGNAME maskeditor )

#include_directories( ${CMAKE_SOURCE_DIR}/src/hugin1 )
#include_directories( ${CMAKE_SOURCE_DIR}/src/maskeditor/core )

	# sources
	set(maskeditor_GUI_SOURCES
				MaskEdApp.h MaskEdApp.cpp
				MaskEdMainFrame.h MaskEdMainFrame.cpp 
				MaskEdClientWnd.h MaskEdClientWnd.cpp 
				MaskEdEditWnd.h MaskEdEditWnd.cpp 
				MaskEdPreviewWnd.h MaskEdPreviewWnd.cpp
				MaskEdThumbnailPreviewCtrl.h MaskEdThumbnailPreviewCtrl.cpp
				MaskEdPrefDlg.h MaskEdPrefDlg.cpp)
	
	IF (WIN32)
	  ADD_EXECUTABLE( ${PROGNAME} WIN32 ${maskeditor_GUI_SOURCES})
	ELSE (WIN32)
	  ADD_EXECUTABLE( ${PROGNAME} ${maskeditor_GUI_SOURCES})
	ENDIF(WIN32)

	target_link_libraries( ${PROGNAME} maskeditor_core maskeditor_segmentation ${common_libs} ${image_libs} huginbasewx ${wxWidgets_LIBRARIES})
	install(TARGETS ${PROGNAME} DESTINATION ${BINDIR})

add_subdirectory(xrc)
