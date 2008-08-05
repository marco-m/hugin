
	# sources
	set(maskeditor_SEGMENTATION_SOURCES
				polyed_basic/PolyEd_Basic.h polyed_basic/PolyEd_Basic.cpp
				polyed_basic/PolyEdBasicMemento.h polyed_basic/PolyEdBasicMemento.cpp
				lazysnapping/LazySnapping.h lazysnapping/LazySnapping.cpp
				lazysnapping/LazySnappingMemento.h lazysnapping/LazySnappingMemento.cpp
				lazysnapping/maxflow/graph.cpp
				lazysnapping/maxflow/maxflow.cpp
				#lazysnapping/kmeans/KMeans.hpp
				lazysnapping/watershed.cxx
				lazysnapping/KMlocal/KCtree.cpp
				lazysnapping/KMlocal/KCutil.cpp
				lazysnapping/KMlocal/KM_ANN.cpp
				lazysnapping/KMlocal/KMcenters.cpp
				lazysnapping/KMlocal/KMdata.cpp
				lazysnapping/KMlocal/KMeans.cpp
				lazysnapping/KMlocal/KMfilterCenters.cpp
				lazysnapping/KMlocal/KMlocal.cpp
				lazysnapping/KMlocal/KMrand.cpp
				lazysnapping/KMlocal/KMterm.cpp)
	
	IF (${HUGIN_SHARED_LIBS})
  	add_library(maskeditor_segmentation SHARED ${maskeditor_SEGMENTATION_SOURCES})
  	set_target_properties(maskeditor_segmentation PROPERTIES VERSION ${HUGIN_LIB_VERSION})
  	install(TARGETS maskeditor_segmentation DESTINATION ${LIBDIR})
	ELSE (${HUGIN_SHARED_LIBS})
  	add_library(maskeditor_segmentation STATIC ${maskeditor_SEGMENTATION_SOURCES})
	ENDIF (${HUGIN_SHARED_LIBS})
