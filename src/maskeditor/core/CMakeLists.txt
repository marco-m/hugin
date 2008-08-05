
	# sources
	set(maskeditor_CORE_SOURCES
				BrushStroke.h
				MaskPoly.h
				MaskMgr.h MaskMgr.cpp
				MaskFileMgr.h MaskFileMgr.cpp
				PTOFileMgr.h PTOFileMgr.cpp 
				MaskEdCommand.h MaskEdCommand.cpp
				IMaskEdMemento.h IMaskEdMemento.cpp
				ISegmentation.h
				Pixel.h Pixel.cpp)
	
	IF (${HUGIN_SHARED_LIBS})
  	add_library(maskeditor_core SHARED ${maskeditor_CORE_SOURCES})
  	set_target_properties(maskeditor_core PROPERTIES VERSION ${HUGIN_LIB_VERSION})
  	install(TARGETS maskeditor_core DESTINATION ${LIBDIR})
	ELSE (${HUGIN_SHARED_LIBS})
  	add_library(maskeditor_core STATIC ${maskeditor_CORE_SOURCES})
	ENDIF (${HUGIN_SHARED_LIBS})
