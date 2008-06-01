
IF(wxWidgets_FOUND)
  IF(WIN32)
    IF ("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
      ADD_DEFINITIONS("-D_UNICODE")
    ENDIF("${wxWidgets_CONFIGURATION}" STREQUAL "mswu")
  ENDIF(WIN32)

  add_subdirectory(gui)
  
ENDIF(wxWidgets_FOUND)

