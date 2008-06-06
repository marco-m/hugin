
SET(XRCFILES 
    main_frame.xrc
    )


INSTALL(FILES ${XRCFILES} DESTINATION ${MASKEDDATADIR}/xrc)

add_subdirectory(data)
