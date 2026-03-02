define_element {
    name          = "Steam",
    tag           = "steam",
    physics       = "gas",
    color         = 0xC8C8C8AA,
    density       = 0.0006,
    viscosity     = 0.0,
    flammability  = 0.0,
    restitution   = 0.0,
    -- Phase 4: condensation
    -- Steam condenses back to water when temperature drops to/below 100 °C (373 K)
    solidify_point = 373.0,
    solidify_into  = "water",
}
