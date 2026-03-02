define_element {
    name                 = "Acid",
    tag                  = "acid",
    physics              = "liquid",
    color                = 0x39FF14DD,
    density              = 1.2,
    viscosity            = 0.01,
    flammability         = 0.0,
    restitution          = 0.05,
    -- Phase 3
    thermal_conductivity = 0.1,
    -- Phase 4: contact reactions — acid dissolves most solid/granular materials
    reactive_with = {
        { target = "stone",  self_into = "",  other_into = "",       prob = 0.5 },
        { target = "sand",   self_into = "",  other_into = "",       prob = 0.6 },
        { target = "wood",   self_into = "",  other_into = "smoke",  prob = 0.55 },
        { target = "ice",    self_into = "water", other_into = "water", prob = 0.8 },
        { target = "gunpowder", self_into = "", other_into = "smoke", prob = 0.7 },
    },
}
