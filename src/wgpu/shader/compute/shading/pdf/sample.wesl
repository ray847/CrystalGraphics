import package::compute::math;
import package::compute::physics;

struct TransportSample {
    next_dir: math::Vec,
    prob: f32,
};

struct EmissionSample {
    next_dir: math::Vec,
    prob: f32,
    radiance: physics::Radiance,
    expected_dist: f32,
};
