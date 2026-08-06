idf_component_register(
    SRCS
        "InitLog.cpp"
        "CCANPGN.cpp"
        "ClientNMEA2000.cpp"
        "CNMEATranslator.cpp"
        "CRouteurEmulateurNMEA.cpp"
    INCLUDE_DIRS "."
)
target_compile_definitions(${COMPONENT_TARGET} PRIVATE _ESP32 _SERIALEMULATOR)
target_compile_options(${COMPONENT_TARGET} PRIVATE -Wno-error)