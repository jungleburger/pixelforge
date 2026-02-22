define_element {
    name          = "Lava",
    tag           = "lava",
    physics       = "liquid",
    color         = 0xFF4000FF,
    density       = 2.2,
    viscosity     = 0.6,
    flammability  = 0.0,
    melting_point = 800.0,    -- freezes below this → stone
    melt_into     = "stone",
    emits_light   = true,
    light_radius  = 4.0,
    light_color   = 0xFF6600FF,
}
