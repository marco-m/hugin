
SET(XRCFILES 
    main_frame.xrc
    )


INSTALL(FILES ${XRCFILES} DESTINATION ${HUGINDATADIR}/xrc)

add_subdirectory(data)
