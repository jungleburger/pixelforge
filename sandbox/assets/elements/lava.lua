define_element {
    name                 = "Lava",
    tag                  = "lava",
    physics              = "liquid",
    color                = 0xFF4000FF,
    density              = 2.2,
    viscosity            = 0.6,
    flammability         = 0.0,
    restitution          = 0.1,
    emits_light          = true,
    light_radius         = 4.0,
    light_color          = 0xFF6600FF,
    -- Phase 3
    heat_output          = 500.0,    -- degrees C per second radiated to neighbours
    thermal_conductivity = 0.5,
    -- Phase 4: solidification
    solidify_point       = 800.0,    -- cools and solidifies to stone below 800 °C
    solidify_into        = "stone",
    -- Phase 4: contact — lava vaporises adjacent water to steam
    reactive_with = {
        { target = "water", self_into = "",    other_into = "steam", prob = 0.8 },
    },
}
