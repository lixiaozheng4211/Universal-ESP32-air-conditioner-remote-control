idf_component_register(
    SRCS
        "main.cpp"
        "ac_catalog.cpp"
        "ac_types.cpp"
        "air_conditioner.cpp"
        "serial_protocol.cpp"
        "drivers/irac_backend.cpp"
        "remotes/gree_remotes.cpp"
        "remotes/haier_remotes.cpp"
        "remotes/midea_remotes.cpp"
    INCLUDE_DIRS "." "drivers" "remotes"
    REQUIRES arduino IRremoteESP8266
)
