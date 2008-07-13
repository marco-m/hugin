# Application name
set( PROGNAME maskeditor )

include_directories( ${CMAKE_SOURCE_DIR}/src/hugin1 )
include_directories( ${CMAKE_SOURCE_DIR}/src/maskeditor/core )
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
				gui/MaskEdThumbnailPreviewCtrl.h gui/MaskEdThumbnailPreviewCtrl.cpp
				gui/MaskEdPrefDlg.h gui/MaskEdPrefDlg.cpp
				core/BrushStroke.h
				core/MaskPoly.h
				core/MaskMgr.h core/MaskMgr.cpp
				core/MaskFileMgr.h core/MaskFileMgr.cpp
				core/PTOFileMgr.h core/PTOFileMgr.cpp 
				core/MaskEdCommand.h core/MaskEdCommand.cpp
				core/ISegmentation.h
				core/Pixel.h core/Pixel.cpp
				segmentation/polyed_basic/PolyEd_Basic.h segmentation/polyed_basic/PolyEd_Basic.cpp
				segmentation/lazysnapping/LazySnapping.h segmentation/lazysnapping/LazySnapping.cpp
				segmentation/lazysnapping/maxflow/graph.cpp
				segmentation/lazysnapping/maxflow/maxflow.cpp
				#segmentation/lazysnapping/kmeans/KMeans.hpp
				segmentation/lazysnapping/watershed.cxx
				segmentation/lazysnapping/KMlocal/KCtree.cpp
				segmentation/lazysnapping/KMlocal/KCutil.cpp
				segmentation/lazysnapping/KMlocal/KM_ANN.cpp
				segmentation/lazysnapping/KMlocal/KMcenters.cpp
				segmentation/lazysnapping/KMlocal/KMdata.cpp
				segmentation/lazysnapping/KMlocal/KMeans.cpp
				segmentation/lazysnapping/KMlocal/KMfilterCenters.cpp
				segmentation/lazysnapping/KMlocal/KMlocal.cpp
				segmentation/lazysnapping/KMlocal/KMrand.cpp
				segmentation/lazysnapping/KMlocal/KMterm.cpp)
	
	IF (WIN32)
	  ADD_EXECUTABLE( ${PROGNAME} WIN32 ${maskeditor_SOURCES})
	ELSE (WIN32)
	  ADD_EXECUTABLE( ${PROGNAME} ${maskeditor_SOURCES} )
	ENDIF(WIN32)

	target_link_libraries( ${PROGNAME} ${common_libs} ${image_libs} huginbasewx ${wxWidgets_LIBRARIES})
	install(TARGETS ${PROGNAME} DESTINATION ${BINDIR})

  add_subdirectory(gui)
  
ENDIF(wxWidgets_FOUND)

