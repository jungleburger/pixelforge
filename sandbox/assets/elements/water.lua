define_element {
    name                 = "Water",
    tag                  = "water",
    physics              = "liquid",
    color                = 0x1E90FFCC,
    density              = 1.0,
    viscosity            = 0.02,
    flammability         = 0.0,
    boiling_point        = 373.0,
    boil_into            = "steam",
    -- Phase 3
    thermal_conductivity = 0.3,
    -- Phase 4: freezing
    solidify_point       = 273.0,    -- freezes to ice at/below 0 °C (273 K)
    solidify_into        = "ice",
}
