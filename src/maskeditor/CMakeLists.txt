include_directories( ${CMAKE_SOURCE_DIR}/src/hugin1 )
include_directories( ${CMAKE_SOURCE_DIR}/src/maskeditor/core )

IF(wxWidgets_FOUND)
  IF(WIN32)
    IF ("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
      ADD_DEFINITIONS("-D_UNICODE")
    ENDIF("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
  ENDIF(WIN32)
	
	add_subdirectory(gui)
	add_subdirectory(core)
	add_subdirectory(segmentation)

ENDIF(wxWidgets_FOUND)