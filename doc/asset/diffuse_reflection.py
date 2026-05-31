import numpy as np
from distribution_function import plot

R = 1


def lambertian(theta, mu):
    return R / np.pi if theta > 0 else np.nan


plot(lambertian)
