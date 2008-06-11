
SET(XRCFILES 
    main_frame.xrc main_menubar.xrc main_toolbar.xrc
    )


INSTALL(FILES ${XRCFILES} DESTINATION ${MASKEDDATADIR}/xrc)

add_subdirectory(data)
