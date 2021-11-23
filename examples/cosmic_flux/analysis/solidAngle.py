import numpy as np


class SolidAngle(object):

    def __init__(self, azimuthal=[0, 2 * np.pi], zenith=[0, 0.5 * np.pi], deg=False):
        if deg:
            self.azimuthal = [np.deg2rad(x) for x in azimuthal]
            self.zenith = [np.deg2rad(x) for x in zenith]
        else:
            self.azimuthal = azimuthal
            self.zenith = zenith

        assert 0 <= self.zenith[0] <= np.pi
        assert 0 <= self.zenith[1] <= np.pi
        assert 0 <= self.azimuthal[0] <= 2 * np.pi
        assert 0 <= self.azimuthal[1] <= 2 * np.pi

        self.value = (self.azimuthal[1] - self.azimuthal[0]) * \
            (np.cos(self.zenith[0]) - np.cos(self.zenith[1]))

    def __mul__(self, other):
        return self.value * other

    def __repr__(self):
        return str(self.value)

    def __int__(self):
        return self.value

    def __add__(self, other):
        return self.value + other

    def __sub__(self, other):
        return self.value - other
