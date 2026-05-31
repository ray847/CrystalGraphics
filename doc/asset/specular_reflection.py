import numpy as np
from distribution_function import plot

R = 1


def specular(theta, mu):
    return R / np.pi if theta > 0 else np.nan


plot(specular)
