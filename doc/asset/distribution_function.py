import inspect
import pathlib
import matplotlib.pyplot as plt
import numpy as np


def plot(dist_func):
    theta = np.linspace(-np.pi / 2, np.pi / 2, 128)
    mu = np.linspace(0, 2 * np.pi, 128)

    mu_v, theta_v = np.meshgrid(mu, theta)
    dist_func_v = np.vectorize(dist_func)

    x = dist_func_v(theta_v, mu_v) * np.cos(theta_v) * np.cos(mu_v)
    y = dist_func_v(theta_v, mu_v) * np.cos(theta_v) * np.sin(mu_v)
    z = dist_func_v(theta_v, mu_v) * np.sin(theta_v)

    # Get metadata.
    frame = inspect.currentframe()
    caller_frame = frame.f_back
    caller_file: str = pathlib.Path(caller_frame.f_code.co_filename).name
    # dist_name = " ".join(
    #     word.capitalize() for word in caller_file[:-3].split("_")
    # )

    fig = plt.figure()
    ax = fig.add_subplot(projection="3d")
    ax.plot_surface(x, y, z)
    ax.set_aspect("equal")
    plt.show()
    fig.savefig(caller_file.replace(".py", ".svg"))
