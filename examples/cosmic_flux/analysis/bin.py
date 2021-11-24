import numpy as np
from solidAngle import SolidAngle
from uncertainties import ufloat
import pickle


class Bin(object):

    def __init__(self, azimuthal=[0, 360], zenith=[0, 90], deg=True):

        self.solidAngle = SolidAngle(azimuthal, zenith, deg=deg)
        self.zenith = zenith
        self.azimuthal = azimuthal
        self.deg = deg
        self.numberOfEntries = 0
        self.tracks = []
        self.time = 0  # in seconds

    def _addTrack(self, t):

        if self.azimuthal[0] <= t.azimuthalAngle <= self.azimuthal[1]:
            if self.zenith[0] <= t.zenithAngle <= self.zenith[1]:
                self.tracks.append(t)

    def addTracks(self, tracks, time):
        self.time += time
        for t in tracks:
            self._addTrack(t)
        #print("Now have", len(self.tracks),"tracks in bin", self.zenith, self.azimuthal)

    def getCOMPoints(self):
        return [t.COMIntersection() for t in self.tracks]

    def plot(self):
        import matplotlib.pyplot as plt

        x = [t.COMIntersection()[0] for t in self.tracks]
        y = [t.COMIntersection()[1] for t in self.tracks]
        plt.scatter(x, y)
        plt.show()

    def realSurface(self):
        return np.cos(np.deg2rad(np.mean(self.zenith))) * 0.3455

    def flux(self):
        area = self.realSurface()
        if area <= 0:
            return ufloat(0, 0)

        n = len(self.tracks)
        # / self.efficiency
        return ufloat(n, n**0.5) / self.time / self.solidAngle.value / area


if __name__ == '__main__':
    b = Bin(azimuthal=[0, 360], zenith=[0, 20], deg=True)
    print(b)
