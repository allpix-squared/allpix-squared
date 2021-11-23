from track import Track
from parse import compile
from tqdm import tqdm
from simulatedTime import SimulatedTime
from histogram import Histogram

"""
Analyse the MCTracks.txt file and plot the observed muon flux
"""

histogram = Histogram(granularity=[1, 15])

trackParser = compile("({:g},{:g},{:g}) ({:g},{:g},{:g})")


def getTracks():
    time = SimulatedTime("cosmic_flux_log.txt").time
    tracks = []
    input = "MCTracks.txt"
    for l in open(input, "r").readlines():
        l = l.strip()
        res = trackParser.parse(l)
        tracks.append(Track(res[0:3], res[3:6], 0, 0, 0, 0))
    return tracks, time


tracks, time = getTracks()

histogram.addTracks(tracks, time)

histogram.printFlux()
histogram.plotZenith()
