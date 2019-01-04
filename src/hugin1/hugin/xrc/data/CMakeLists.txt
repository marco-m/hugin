
FILE(GLOB DATAFILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.htm *.html *.ico *.jpg *.png tips.txt upstream.txt)
LIST(REMOVE_ITEM DATAFILES hugin_128.png hugin_256.png ptbatcher_128.png ptbatcher_256.png)
INSTALL(FILES ${DATAFILES} DESTINATION ${HUGINDATADIR}/xrc/data)

# parse contributors list
file(READ "${PROJECT_SOURCE_DIR}/authors.txt" AUTHORS_LIST)

# replace tab with next table cell
string(REGEX REPLACE "\t"
         "</td><td>" AUTHORS_LIST "${AUTHORS_LIST}")
# replace newline with next table row
string(REGEX REPLACE "\n"
         "</td></tr><tr><td>" AUTHORS_LIST "${AUTHORS_LIST}")
# remove extra last row
string(REGEX REPLACE "<tr><td>$"
         "" AUTHORS_LIST "${AUTHORS_LIST}")

configure_file(about.htm.in ${CMAKE_BINARY_DIR}/src/hugin1/hugin/xrc/data/about.htm)
INSTALL(FILES ${CMAKE_BINARY_DIR}/src/hugin1/hugin/xrc/data/about.htm DESTINATION ${HUGINDATADIR}/xrc/data)

# license text for about dialog
INSTALL(FILES ${CMAKE_SOURCE_DIR}/COPYING.txt DESTINATION ${HUGINDATADIR}/xrc/data/)

IF (UNIX)
  INSTALL(FILES hugin.png ptbatcher.png DESTINATION ${DATADIR}/icons/48x48/apps)
  INSTALL(FILES hugin_128.png DESTINATION ${DATADIR}/icons/128x128/apps RENAME hugin.png)
  INSTALL(FILES hugin_256.png DESTINATION ${DATADIR}/icons/256x256/apps RENAME hugin.png)
  INSTALL(FILES ptbatcher_128.png DESTINATION ${DATADIR}/icons/128x128/apps RENAME ptbatcher.png)
  INSTALL(FILES ptbatcher_256.png DESTINATION ${DATADIR}/icons/256x256/apps RENAME ptbatcher.png)
  INSTALL(FILES hugin.svg ptbatcher.svg DESTINATION ${DATADIR}/icons/hicolor/scalable/apps)
ENDIF (UNIX)

# install cursor on windows
IF(WIN32)
  INSTALL(FILES cursor_cp_pick.cur DESTINATION ${HUGINDATADIR}/xrc/data/)
ENDIF(WIN32)

INSTALL(FILES expressions.ini DESTINATION ${HUGINDATADIR}/data)

ADD_SUBDIRECTORY(help_en_EN)
