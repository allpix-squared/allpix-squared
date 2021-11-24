from track import Track
from tqdm import tqdm
from simulatedTime import SimulatedTime
from histogram import Histogram
import argparse
import os
import os.path as path
import ROOT
from ROOT import gSystem

"""
Analyse the cosmicsMC.root file and plot the observed muon flux
"""


# argument parser
parser = argparse.ArgumentParser()
parser.add_argument("-l", metavar='libAllpixObjects', required=False,
                    help="specify path to the libAllpixObjects library (generally in allpix-squared/lib/)) ")
args = parser.parse_args()

if args.l is not None:  # Try to find Allpix Library
    lib_file_name = (str(args.l))
else:  # Look in LD_LIBRARY_PATH
    libraryPaths = os.environ['LD_LIBRARY_PATH'].split(':')
    for p in libraryPaths:
        if p.endswith("lib/libAllpixObjects.so"):
            lib_file_name = p
            break

if (not os.path.isfile(lib_file_name)):
    print("WARNING: ", lib_file_name, " does not exist, exiting")
    exit(1)

histogram = Histogram(granularity=[1, 15])

# load library and rootfile
gSystem.Load(lib_file_name)
rootFile = ROOT.TFile("output/cosmicsMC.root")
McParticle = rootFile.Get('MCParticle')


def getTracks():
    "Load tracks from the ROOT file"
    time = SimulatedTime("cosmic_flux_log.txt").time
    tracks = []

    for i in tqdm(range(0, McParticle.GetEntries())):
        McParticle.GetEntry(i)
        McParticle_branch = McParticle.GetBranch("telescope0_0")
        br_mc_part = getattr(McParticle, McParticle_branch.GetName())

        startPoint = None
        endPoint = None

        for mc_part in br_mc_part:
            # Only consider muons
            if abs(mc_part.getParticleID()) == 13:
                startPoint = mc_part.getLocalStartPoint()
                endPoint = mc_part.getLocalEndPoint()
                direction = startPoint - endPoint
                direction = direction / direction.z()
                tracks.append(Track([startPoint.x(), startPoint.y(), startPoint.z()], [
                              direction.x(), direction.y(), direction.z()], 0, 0, 0, 0))
    return tracks, time


# Get tracks from file
tracks, time = getTracks()
histogram.addTracks(tracks, time)

# Do the analysis and show the plot
histogram.printFlux()
histogram.plotZenith()
