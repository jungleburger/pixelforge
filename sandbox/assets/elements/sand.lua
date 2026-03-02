define_element {
    name                 = "Sand",
    tag                  = "sand",
    physics              = "powder",
    color                = 0xC2B280FF,
    density              = 1.6,
    viscosity            = 0.0,
    flammability         = 0.0,
    restitution          = 0.3,
    melting_point        = 1700.0,   -- glass transition (no glass element yet; melts to lava)
    melt_into            = "lava",
    -- Phase 3
    thermal_conductivity = 0.05,
}
