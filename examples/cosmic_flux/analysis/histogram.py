from bin import Bin
import numpy as np
import matplotlib.pyplot as plt


class Histogram(object):
    def __init__(self, azimuthal=[0, 360], zenith=[0, 90], deg=True, granularity=[4, 5]):
        self.azimuthalLinspace = np.linspace(
            azimuthal[0], azimuthal[1], num=granularity[0] + 1, endpoint=True)
        self.zenithLinspace = np.linspace(
            zenith[0], zenith[1], num=granularity[1] + 1, endpoint=True)
        self.bins = [[None for j in range(granularity[1])]
                     for i in range(granularity[0])]

        for i in range(granularity[0]):
            for j in range(granularity[1]):
                azimuthalRange = [self.azimuthalLinspace[i],
                                  self.azimuthalLinspace[i + 1]]
                zenithRange = [self.zenithLinspace[j],
                               self.zenithLinspace[j + 1]]
                b = Bin(azimuthal=azimuthalRange, zenith=zenithRange, deg=deg)
                self.bins[i][j] = b
        self.granularity = granularity

    def addTracks(self, tracks, time):
        for a in self.bins:
            for b in a:
                b.addTracks(tracks, time)

    def plot(self):
        for a in self.bins:
            for b in a:
                b.getCOMPoints()

    def printFlux(self):
        for a in self.bins:
            for b in a:
                print("Bin", b.zenith, b.azimuthal,
                      ":", b.flux(), "Muons/s/m^2/sr")

    def plotZenith(self):
        assert self.granularity[0] == 1
        x = [(self.zenithLinspace[i] + self.zenithLinspace[i + 1]) /
             2 for i in range(len(self.zenithLinspace) - 1)]
        xerr = [(self.zenithLinspace[i + 1] - self.zenithLinspace[i]
                 ) / 2 for i in range(len(self.zenithLinspace) - 1)]
        flux = []
        for a in self.bins:
            for b in a:
                flux.append(b.flux())
        y = [f.n for f in flux]
        yerr = [f.s for f in flux]
        print(x)
        plt.errorbar(x, y, xerr=xerr, yerr=yerr, linestyle="None", marker='.')

        xcos = np.linspace(
            self.zenithLinspace[0], self.zenithLinspace[-1], 200, endpoint=True)
        ycos = y[0] * np.cos(np.deg2rad(xcos))**2
        #plt.plot(xcos, ycos)
        plt.xlabel(r"Zenith Angle [$^\circ$]")
        plt.ylabel(r"Muon Flux [Muons $s^{-1}$ $sr^{-1}$ $m^{-2}$]")
        x = np.linspace(0, 90, 100)
        y = np.cos(np.deg2rad(x))**2 * max(y)
        plt.plot(x, y)

        plt.savefig("ZenithFlux.pdf")
        plt.show()
