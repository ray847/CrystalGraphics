var<private> rng_state: u32;

fn seedRng(seed: u32) {
    rng_state = seed;
    stepRng();
}

fn stepRng() {
    rng_state = rng_state * 747796405u + 2891336453u;
    var word: u32 = ((rng_state >> ((rng_state >> 28u) + 4u)) ^ rng_state) * 277803737u;
    rng_state = (word >> 22u) ^ word;
}

fn randFloat() -> f32 {
    stepRng();
    return f32(rng_state) / 4294967296.0;
}