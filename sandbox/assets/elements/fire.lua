define_element {
    name                 = "Fire",
    tag                  = "fire",
    physics              = "fire",
    color                = 0xFF4500FF,
    density              = 0.01,
    viscosity            = 0.0,
    flammability         = 0.0,
    restitution          = 0.0,
    boiling_point        = 1200.0,   -- fire "boils out" (extinguishes at extreme heat)
    boil_into            = "smoke",
    emits_light          = true,
    light_radius         = 8.0,
    light_color          = 0xFF8800FF,
    -- Phase 3
    heat_output          = 800.0,    -- degrees C per second radiated to neighbours
    thermal_conductivity = 0.0,      -- fire is a gas; no solid conduction
}
