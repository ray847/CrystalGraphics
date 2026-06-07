const SQRT_HALF_PI: f32 = 1.2533141373155001f;
const INV_SQRT_TWO: f32 = 0.7071067811865475f;

fn xyzMatch(wavelength: f32) -> vec3f {
    return vec3f(
        xMatch(wavelength),
        yMatch(wavelength),
        zMatch(wavelength)
    );
}

fn xMatch(wavelength: f32) -> f32 {
    return 0.362f * splitGaussian(wavelength, 442.0f, 1.0f / 0.0624f, 1.0f / 0.0374f)
        + 1.056f * splitGaussian(wavelength, 599.8f, 1.0f / 0.0264f, 1.0f / 0.0323f)
        - 0.065f * splitGaussian(wavelength, 501.1f, 1.0f / 0.0490f, 1.0f / 0.0382f);
}

fn yMatch(wavelength: f32) -> f32 {
    return 0.821f * splitGaussian(wavelength, 568.8f, 1.0f / 0.0213f, 1.0f / 0.0247f)
        + 0.286f * splitGaussian(wavelength, 530.9f, 1.0f / 0.0613f, 1.0f / 0.0322f);
}

fn zMatch(wavelength: f32) -> f32 {
    return 1.217f * splitGaussian(wavelength, 437.0f, 1.0f / 0.0845f, 1.0f / 0.0278f)
        + 0.681f * splitGaussian(wavelength, 459.0f, 1.0f / 0.0385f, 1.0f / 0.0725f);
}

fn xyzMatchSum(wavelength: f32) -> f32 {
    let xyz = xyzMatch(wavelength);
    return xyz.x + xyz.y + xyz.z;
}

fn xyzMatchSumIntegral(low: f32, high: f32) -> f32 {
    return xIntegral(low, high) + yIntegral(low, high) + zIntegral(low, high);
}

fn xIntegral(low: f32, high: f32) -> f32 {
    return 0.362f * splitGaussianIntegral(
            low,
            high,
            442.0f,
            1.0f / 0.0624f,
            1.0f / 0.0374f
        )
        + 1.056f * splitGaussianIntegral(
            low,
            high,
            599.8f,
            1.0f / 0.0264f,
            1.0f / 0.0323f
        )
        - 0.065f * splitGaussianIntegral(
            low,
            high,
            501.1f,
            1.0f / 0.0490f,
            1.0f / 0.0382f
        );
}

fn yIntegral(low: f32, high: f32) -> f32 {
    return 0.821f * splitGaussianIntegral(
            low,
            high,
            568.8f,
            1.0f / 0.0213f,
            1.0f / 0.0247f
        )
        + 0.286f * splitGaussianIntegral(
            low,
            high,
            530.9f,
            1.0f / 0.0613f,
            1.0f / 0.0322f
        );
}

fn zIntegral(low: f32, high: f32) -> f32 {
    return 1.217f * splitGaussianIntegral(
            low,
            high,
            437.0f,
            1.0f / 0.0845f,
            1.0f / 0.0278f
        )
        + 0.681f * splitGaussianIntegral(
            low,
            high,
            459.0f,
            1.0f / 0.0385f,
            1.0f / 0.0725f
        );
}

fn splitGaussian(
    wavelength: f32,
    center: f32,
    left_width: f32,
    right_width: f32
) -> f32 {
    let width = select(right_width, left_width, wavelength < center);
    let scaled = (wavelength - center) / width;
    return exp(-0.5f * scaled * scaled);
}

fn splitGaussianIntegral(
    low: f32,
    high: f32,
    center: f32,
    left_width: f32,
    right_width: f32
) -> f32 {
    return splitGaussianAntiderivative(high, center, left_width, right_width)
        - splitGaussianAntiderivative(low, center, left_width, right_width);
}

fn splitGaussianAntiderivative(
    wavelength: f32,
    center: f32,
    left_width: f32,
    right_width: f32
) -> f32 {
    if wavelength < center {
        return SQRT_HALF_PI * left_width
            * (1.0f + erfApprox((wavelength - center) * INV_SQRT_TWO / left_width));
    }
    return SQRT_HALF_PI * left_width
        + SQRT_HALF_PI * right_width
        * erfApprox((wavelength - center) * INV_SQRT_TWO / right_width);
}

fn erfApprox(x: f32) -> f32 {
    let sign = select(-1.0f, 1.0f, x >= 0.0f);
    let ax = abs(x);
    let t = 1.0f / (1.0f + 0.3275911f * ax);
    let poly = (((((1.061405429f * t - 1.453152027f) * t)
        + 1.421413741f) * t - 0.284496736f) * t
        + 0.254829592f) * t;
    return sign * (1.0f - poly * exp(-ax * ax));
}
