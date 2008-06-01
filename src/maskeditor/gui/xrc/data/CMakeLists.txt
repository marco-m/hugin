
FILE(GLOB DATAFILES *.htm *.html *.ico *.jpg *.png *.xpm *.txt *.mk)
INSTALL(FILES ${DATAFILES} DESTINATION ${MASKEDDATADIR}/xrc/data)

