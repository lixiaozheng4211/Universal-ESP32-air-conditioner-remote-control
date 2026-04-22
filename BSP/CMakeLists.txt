set(src_dirs
  IR_NEC_ENCODER
)

set(include_dirs
  IR_NEC_ENCODER
)

set(requires
  esp_driver_rmt
)

idf_component_register(
  SRC_DIRS ${src_dirs}
  INCLUDE_DIRS ${include_dirs}
  REQUIRES ${requires}
)
