#ROOT imports
import ROOT
from ROOT import TFile, gDirectory,gSystem,TClass

import os, sys
import argparse

#numpy
import numpy as np
import numpy.ma as ma

#matplotlib
import matplotlib.pyplot as plt
from matplotlib import pyplot
from matplotlib.colors import LogNorm
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import cm
from matplotlib.colors import LinearSegmentedColormap

#scipy
from scipy import stats

#argument parser
parser = argparse.ArgumentParser()
parser.add_argument("-l", metavar='libAllpixObjects',required=True, help="specify path to the libAllpixObjects library (generally in allpix-squared/lib/)) ")
parser.add_argument("-d", metavar='detector', required=True, help="specify your detector name (generally detector1, dut, ...) ")
parser.add_argument("-f", metavar='rootfile', required=True, help="specify path to the rootfile to be processed ")


args=parser.parse_args()

root_file_name=(str(args.f))
lib_file_name=(str(args.l))
detector_name=(str(args.d))

if (not os.path.isfile(lib_file_name)):
    print "WARNING: ", lib_file_name, " does not exist, exiting"
    exit(1)
if (not os.path.isfile(root_file_name)):
    print "WARNING: ", root_file_name, " does not exist, exiting"
    exit(1)

#load library and rootfile
gSystem.Load(lib_file_name)
rootfile=ROOT.TFile(root_file_name)
gDirectory.ls()

McParticle= rootfile.Get('MCParticle')
PixelCharge= rootfile.Get('PixelCharge')
PropCharge= rootfile.Get('PropagatedCharge')
PixelHit= rootfile.Get('PixelHit')

#example of python dictionnaries to fill
mc_global_endpoints={'x': [],'y': [], 'z': []}
mc_local_endpoints={'x': [],'y': [], 'z': []}
pixel_hit={'x': [],'y': [], 'signal': []}

#loop on pixel hit branch
empty_mc_branch=0
for iev in range(0, PixelHit.GetEntries()):
    PixelHit.GetEntry(iev)
    PixelCharge.GetEntry(iev)
    McParticle.GetEntry(iev)
    PixelCharge_branch=PixelCharge.GetBranch(detector_name)
    PixelHit_branch=PixelHit.GetBranch(detector_name)
    McParticle_branch=McParticle.GetBranch(detector_name)
    
    if(not PixelCharge_branch):
        print "WARNING: cannot find PixelCharge branch in the TTree with detector name: "+ detector_name+",  exiting"
        exit(1)
    print ' processing event number {0}\r'.format(iev), "out of", PixelHit.GetEntries(), "events",

    #assign AP2 vectors to branches
    br_pix_charge = getattr(PixelCharge, PixelCharge_branch.GetName()) 
    br_pix_hit = getattr(PixelHit, PixelHit_branch.GetName())
    br_mc_part = getattr(McParticle, McParticle_branch.GetName())
   
    if br_mc_part.size()<1:
        empty_mc_branch+=1;
        continue

    output_track_x=0.
    output_track_y=0.
    output_track_z=0.

    output_track_x_global=0.
    output_track_y_global=0.
    output_track_z_global=0.
    for mc_part in br_mc_part:
        output_track_x+=mc_part.getLocalEndPoint().x()
        output_track_y+=mc_part.getLocalEndPoint().y()
        output_track_z+=mc_part.getLocalEndPoint().z()

        output_track_x_global+=mc_part.getGlobalEndPoint().x()
        output_track_y_global+=mc_part.getGlobalEndPoint().y()
        output_track_z_global+=mc_part.getGlobalEndPoint().z()

    output_track_x/=float(br_mc_part.size())
    output_track_y/=float(br_mc_part.size())
    output_track_z/=float(br_mc_part.size())

    output_track_x_global/=float(br_mc_part.size())
    output_track_y_global/=float(br_mc_part.size())
    output_track_z_global/=float(br_mc_part.size())

    for pix_hit in br_pix_hit:
        pixel_hit['x'].append(pix_hit.getPixel().getLocalCenter().x())
        pixel_hit['y'].append(pix_hit.getPixel().getLocalCenter().y())
        pixel_hit['signal'].append(pix_hit.getSignal())

        mc_global_endpoints['x'].append(output_track_x_global)
        mc_global_endpoints['y'].append(output_track_y_global)
        mc_global_endpoints['z'].append(output_track_z_global)

        mc_local_endpoints['x'].append(output_track_x)
        mc_local_endpoints['y'].append(output_track_y)
        mc_local_endpoints['z'].append(output_track_z)

#print numner of events processed
print " ----- processed events (pixelhit):", PixelHit.GetEntries(), " empty_mc_branch:", empty_mc_branch

# plot pixel collected charge versus MC parcticle endpoint coordinates
mc_hit_histogram=plt.figure(figsize=(11,7))
H2, xedges, yedges, binnmmber = stats.binned_statistic_2d(mc_local_endpoints['x'], mc_local_endpoints['y'], values = pixel_hit['signal'], statistic='mean' , bins = [100, 100])
XX, YY = np.meshgrid(xedges, yedges)
Hm = ma.masked_where(np.isnan(H2),H2)
plt.pcolormesh(XX,YY,Hm,cmap="inferno")
plt.ylabel("pixel y [mm]")
plt.xlabel("pixel x [mm]")
cbar = plt.colorbar(pad = .015, aspect=20)
cbar.set_label("hit signal")

mc_hit_histogram_z=plt.figure(figsize=(11,7))
H2, xedges, yedges, binnmmber = stats.binned_statistic_2d(mc_local_endpoints['x'], mc_local_endpoints['z'], values = pixel_hit['signal'], statistic='mean' , bins = [100, 100])
XX, YY = np.meshgrid(xedges, yedges)
Hm = ma.masked_where(np.isnan(H2),H2)
plt.pcolormesh(XX,YY,Hm.T,cmap="inferno")
plt.ylabel("pixel z [mm]")
plt.xlabel("pixel x [mm]")
cbar = plt.colorbar(pad = .015, aspect=20)
cbar.set_label("hit signal ")


figMC_z = plt.figure()
plt.hist(mc_local_endpoints['z'],100)
plt.title("MC global end point z coordinate")
plt.xlabel("z_global [mm]")

figMC_z_log = plt.figure()
plt.hist(mc_global_endpoints['x'],100)
plt.title("MC global end point z coordinate")
plt.xlabel("z_global [mm]")
plt.yscale('log')

# 3D plot x,y,z MC particle endpoint with color scale representing the amount of charge (quite heavy)
fig3D = plt.figure()
ax = fig3D.add_subplot(111, projection='3d')
colors = LinearSegmentedColormap('colormap', cm.jet._segmentdata.copy(), np.max(pixel_hit['signal']))
plot=ax.scatter(mc_local_endpoints['x'], mc_local_endpoints['y'], mc_local_endpoints['z'],c=pixel_hit['signal'],cmap="jet",marker=".",s=2)#,c= marker=m)
ax.set_ylabel("y mc (mm)")
ax.set_xlabel("x mc (mm)")
ax.set_zlabel("z mc (mm)")
cbar=fig3D.colorbar(plot , ax=ax)
cbar.set_label("hit signal")

#show plots
plt.show()
