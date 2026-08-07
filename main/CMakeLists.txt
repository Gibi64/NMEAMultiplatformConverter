idf_component_register(
    SRCS
        "InitLog.cpp"
        "CCANPGN.cpp"
        "ClientNMEA2000.cpp"
        "CNMEATranslator.cpp"
        "CRouteurEmulateurNMEA.cpp"
    INCLUDE_DIRS "."
)
if(NOT DEFINED NMEA_TIMEFACTOR)
    set(NMEA_TIMEFACTOR 1.0)
endif()

target_compile_definitions(${COMPONENT_TARGET} PRIVATE _ESP32 _SERIALEMULATOR _TIMEFACTOR=${NMEA_TIMEFACTOR})
target_compile_options(${COMPONENT_TARGET} PRIVATE -Wno-error)