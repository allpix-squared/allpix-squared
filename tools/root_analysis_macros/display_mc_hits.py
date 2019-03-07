#ROOT imports
import ROOT
from ROOT import gROOT, TCanvas, TF1, gApplication
from ROOT import TFile, TColor
from ROOT import TH1F,TH2F,TGraph
from ROOT import kBlack, kBlue, kRed, kCyan, kMagenta, kGreen, kGray
from ROOT import TLegend
from ROOT import gStyle
from ROOT import gDirectory,gSystem
from ROOT import std

import os, sys

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

gStyle.SetPalette(109)

gSystem.Load("/path/to/libAllpixObjects.dylib")
filename="/path/to/data.root"

if (not os.path.isfile(filename)):
    print "WARNING: ", filename, " does not exist, exiting"
    exit()
rootfile=ROOT.TFile(filename)
gDirectory.ls()

mc_particle= rootfile.Get('MCParticle')
pixel_charge= rootfile.Get('PixelCharge')
prop_charge= rootfile.Get('PropagatedCharge')
pixel_hit= rootfile.Get('PixelHit')
mc_particle.Print()

hit_x=np.array([])
hit_y=np.array([])
hit_signal=np.array([])
np_pix_charge=np.array([])
mc_local_endpoint_x=np.array([])
mc_local_endpoint_y=np.array([])
mc_local_endpoint_z=np.array([])

mc_global_endpoint_x=np.array([])
mc_global_endpoint_y=np.array([])
mc_global_endpoint_z=np.array([])

#loop on pixel hit branch
empty_mc_branch=0
for iev in range(0, pixel_hit.GetEntries()):
    print ' processing event number {0}\r'.format(iev), "out of", pixel_hit.GetEntries(), "events",#/ float(pixel_hit.GetEntries()),
    pixel_hit.GetEntry(iev)
    pixel_charge.GetEntry(iev)
    mc_particle.GetEntry(iev)

    pixel_charge_branch= pixel_charge.detector
    pixel_hit_branch= pixel_hit.detector
    mc_particle_branch= mc_particle.detector

#skip events which have no mc particles associated
    if mc_particle_branch.size()<1:
        empty_mc_branch+=1;
        continue

    output_track_x=0.
    output_track_y=0.
    output_track_z=0.

    output_track_x_global=0.
    output_track_y_global=0.
    output_track_z_global=0.
    for mc_part in mc_particle_branch:
        output_track_x+=mc_part.getLocalEndPoint().x()
        output_track_y+=mc_part.getLocalEndPoint().y()
        output_track_z+=mc_part.getLocalEndPoint().z()

        output_track_x_global+=mc_part.getGlobalEndPoint().x()
        output_track_y_global+=mc_part.getGlobalEndPoint().y()
        output_track_z_global+=mc_part.getGlobalEndPoint().z()

    output_track_x/=float(mc_particle_branch.size())
    output_track_y/=float(mc_particle_branch.size())
    output_track_z/=float(mc_particle_branch.size())

    output_track_x_global/=float(mc_particle_branch.size())
    output_track_y_global/=float(mc_particle_branch.size())
    output_track_z_global/=float(mc_particle_branch.size())

    for pix_hit in pixel_hit_branch:
        hit_x=np.append(hit_x,pix_hit.getPixel().getLocalCenter().x())
        hit_y=np.append(hit_y,pix_hit.getPixel().getLocalCenter().y())
        hit_signal=np.append(hit_signal,pix_hit.getSignal())
        mc_local_endpoint_x=np.append(mc_local_endpoint_x,output_track_x)
        mc_local_endpoint_y=np.append(mc_local_endpoint_y,output_track_y)
        mc_local_endpoint_z=np.append(mc_local_endpoint_z,output_track_z)

        mc_global_endpoint_x=np.append(mc_global_endpoint_x,output_track_x)
        mc_global_endpoint_y=np.append(mc_global_endpoint_y,output_track_y)
        mc_global_endpoint_z=np.append(mc_global_endpoint_z,output_track_z)

#print numner of events processed
print " ----- processed events (pixelhit):", pixel_hit.GetEntries(), " empty_mc_branch:", empty_mc_branch

#--- plot some output histograms----

# hit signal
histograms=plt.figure()
plt.subplot(221)
plt.hist(hit_x,100)
plt.subplot(222)
plt.hist(hit_signal,100)
plt.yscale('log')
plt.subplot(223)
plt.hist2d(hit_x,hit_y,100)

# plot pixel collected charge versus MC parcticle endpoint coordinates
mc_hit_histogram=plt.figure(figsize=(11,7))
H2, xedges, yedges, binnmmber = stats.binned_statistic_2d(mc_local_endpoint_x, mc_local_endpoint_y, values = hit_signal, statistic='mean' , bins = [150, 150])
XX, YY = np.meshgrid(xedges, yedges)
Hm = ma.masked_where(np.isnan(H2),H2)
plt.pcolormesh(XX,YY,Hm,cmap="inferno")
plt.ylabel("pixel x [mm]")
plt.xlabel("pixel y [mm]")
cbar = plt.colorbar(pad = .015, aspect=20)
cbar.set_label("collected charge (e)")

mc_hit_histogram_z=plt.figure(figsize=(11,7))
H2, xedges, yedges, binnmmber = stats.binned_statistic_2d(mc_local_endpoint_x, mc_local_endpoint_z, values = hit_signal, statistic='mean' , bins = [150, 150])
XX, YY = np.meshgrid(xedges, yedges)
Hm = ma.masked_where(np.isnan(H2),H2)
plt.pcolormesh(XX,YY,Hm.T,cmap="inferno")
plt.ylabel("pixel x [mm]")
plt.xlabel("pixel z [mm]")
cbar = plt.colorbar(pad = .015, aspect=20)
cbar.set_label("collected charge (e)")


figMC_z = plt.figure()
plt.hist(mc_global_endpoint_z,100)
plt.title("MC global end point z coordinate")
plt.xlabel("z_global [mm]")

figMC_z_log = plt.figure()
plt.hist(mc_global_endpoint_z,100)
plt.title("MC global end point z coordinate")
plt.xlabel("z_global [mm]")
plt.yscale('log')

# 3D plot x,y,z MC particle endpoint with color scale representing the amount of charge (quite heavy may want to comment)
fig3D = plt.figure()
ax = fig3D.add_subplot(111, projection='3d')
colors = LinearSegmentedColormap('colormap', cm.jet._segmentdata.copy(), np.max(hit_signal))
plot=ax.scatter(mc_local_endpoint_x, mc_local_endpoint_y, mc_local_endpoint_z,c=hit_signal,cmap="jet",marker=".",s=2)#,c= marker=m)
ax.set_ylabel("y mc (mm)")
ax.set_xlabel("x mc (mm)")
ax.set_zlabel("z mc (mm)")
cbar=fig3D.colorbar(plot , ax=ax)
cbar.set_label("collected charge (e)")

#show plots
plt.show()
