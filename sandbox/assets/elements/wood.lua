define_element {
    name                 = "Wood",
    tag                  = "wood",
    physics              = "solid",
    color                = 0x8B4513FF,
    density              = 0.6,
    viscosity            = 1.0,
    flammability         = 0.4,
    restitution          = 0.3,
    melting_point        = 573.0,
    melt_into            = "fire",
    -- Phase 3
    ignition_point       = 300.0,    -- catches fire if temperature reaches 300 °C
    thermal_conductivity = 0.05,
    ash_into             = "stone",  -- charred wood → stone (closest to ash we have)
}
