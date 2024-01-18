# Create Plots to visualize MC Data from Allpix Squared

# SPDX-FileCopyrightText: 2019-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# ===TO DO== #
# Create Batch processing for multiple detectors

# ROOT imports
import ROOT
from ROOT import TFile, gDirectory, gSystem, TClass

import os, sys
import argparse
import os.path as path

# numpy
import numpy as np
import numpy.ma as ma

# matplotlib
import matplotlib

# scipy
from scipy import stats
from scipy.stats import norm

# argument parser
parser = argparse.ArgumentParser()
parser.add_argument("-l", metavar='libAllpixObjects', required=False,
                    help="specify path to the libAllpixObjects library (generally in allpix-squared/lib/)) ")
parser.add_argument("-d", metavar='detector', required=True,
                    help="specify your detector name (generally detector1, dut, ...) ")
parser.add_argument("-f", metavar='rootfile', required=True, help="specify path to the rootfile to be processed ")

parser.add_argument("-a", "--all", required=False, help="Produce All Graphs", action="store_true")

parser.add_argument("--pdf", help="Create PDF rather than pop up (Useful for Docker)", action="store_true")

parser.add_argument("-v", "--verbose", help="Toggle verbose settings", action="store_true")

parser.add_argument("--onedim", "--1d", help="Print all one dimensional plots (Histograms)", action="store_true")

parser.add_argument("--twodim", "--2d", help="Print all two dimensional plots (Heatmaps)", action="store_true")

parser.add_argument("--threedim", "--3d", help="Print all Three dimensional plots", action="store_true")

parser.add_argument("-g", "--gaussian", "--norm", help="Print histograms with gaussian curve", action="store_true")

parser.add_argument("-c", "--cauchy", "--lorentz", help="Print histograms with Cauchy-Lorentz curve", action="store_true")


args = parser.parse_args()

root_file_name = (str(args.f))
detector_name = (str(args.d))
print_all = args.all
print_1d = args.onedim
print_2d = args.twodim
print_3d = args.threedim
print_gauss = args.gaussian
print_cauch = args.cauchy
save_pdf = args.pdf
verbose = args.verbose
outDir = os.path.dirname(root_file_name)

if not (print_1d or print_2d or print_3d or print_gauss or print_cauch):
    print_og = True
else:
    print_og = False

if save_pdf:
    matplotlib.use('pdf')

import matplotlib.pyplot as plt # These imports must be called after the use pdf argument
from matplotlib import pyplot
from matplotlib.colors import LogNorm
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import cm
from matplotlib.colors import LinearSegmentedColormap

if args.l is not None:  # Try to find Allpix Library
    lib_file_name = (str(args.l))
    if (not os.path.isfile(lib_file_name)):
        print("WARNING: ", lib_file_name, " does not exist, exiting")
        exit(1)

elif os.path.isfile(path.abspath(path.join(__file__ ,"..","..","opt","allpix-squared","lib","libAllpixObjects.so"))): # For native installs
    lib_file_name = path.abspath(path.join(__file__ ,"..","..","opt","allpix-squared","lib","libAllpixObjects.so"))

elif os.path.isfile(path.join(path.sep, "opt","allpix-squared","lib","libAllpixObjects.so")): # For Docker installs
    lib_file_name = path.join(path.sep, "opt","allpix-squared","lib","libAllpixObjects.so")

else:
    print("WARNING: No Allpix Objects Library found, exiting")
    exit(1)


if (not os.path.isfile(lib_file_name)):
    print("WARNING: no allpix library found, exiting (Use -l to manually set location of libraries)")
    exit(1)
if (not os.path.isfile(root_file_name)):
    print("WARNING: " + root_file_name + " does not exist, exiting")
    exit(1)

# load library and rootfile
gSystem.Load(lib_file_name)
rootfile = ROOT.TFile(root_file_name)
gDirectory.ls()

# Check on detector existence
if not rootfile.GetDirectory("detectors/" + detector_name):
    print("\nDetector does not exist. Please choose one of the following detectors:")
    gDirectory.cd("detectors")
    list_of_keys = gDirectory.GetListOfKeys()
    for key in list_of_keys:
        print(key.GetName())
    exit(1)

McParticle = rootfile.Get('MCParticle')
PixelCharge = rootfile.Get('PixelCharge')
PropCharge = rootfile.Get('PropagatedCharge')
PixelHit = rootfile.Get('PixelHit')

# example of python dictionaries to fill
mc_global_endpoints = {'x': [], 'y': [], 'z': []}
mc_local_endpoints = {'x': [], 'y': [], 'z': []}
pixel_hit = {'x': [], 'y': [], 'signal': []}

# loop on pixel hit branch
empty_mc_branch = 0
print("Loading in Data...")
for iev in range(0, PixelHit.GetEntries()):
    PixelHit.GetEntry(iev)
    PixelCharge.GetEntry(iev)
    McParticle.GetEntry(iev)
    PixelCharge_branch = PixelCharge.GetBranch(detector_name)
    PixelHit_branch = PixelHit.GetBranch(detector_name)
    McParticle_branch = McParticle.GetBranch(detector_name)

    if (not PixelCharge_branch):
        Warning("WARNING: cannot find PixelCharge branch in the TTree with detector name: " + detector_name + ",  exiting")
        exit(1)
    if verbose: print(' processing event number {0}\r'.format(iev), "out of", PixelHit.GetEntries(), "events",)

    # assign AP2 vectors to branches
    br_pix_charge = getattr(PixelCharge, PixelCharge_branch.GetName())
    br_pix_hit = getattr(PixelHit, PixelHit_branch.GetName())
    br_mc_part = getattr(McParticle, McParticle_branch.GetName())

    if br_mc_part.size() < 1:
        empty_mc_branch += 1;
        continue

    output_track_x = 0.
    output_track_y = 0.
    output_track_z = 0.

    output_track_x_global = 0.
    output_track_y_global = 0.
    output_track_z_global = 0.
    for mc_part in br_mc_part:
        output_track_x += mc_part.getLocalEndPoint().x()
        output_track_y += mc_part.getLocalEndPoint().y()
        output_track_z += mc_part.getLocalEndPoint().z()

        output_track_x_global += mc_part.getGlobalEndPoint().x()
        output_track_y_global += mc_part.getGlobalEndPoint().y()
        output_track_z_global += mc_part.getGlobalEndPoint().z()

    output_track_x /= float(br_mc_part.size())
    output_track_y /= float(br_mc_part.size())
    output_track_z /= float(br_mc_part.size())

    output_track_x_global /= float(br_mc_part.size())
    output_track_y_global /= float(br_mc_part.size())
    output_track_z_global /= float(br_mc_part.size())

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

# print number of events processed
print(" ----- processed events (pixelhit):" + str(PixelHit.GetEntries()) + " empty_mc_branch:" + str(empty_mc_branch))

print("Plotting...")

# plot pixel collected charge versus MC parcticle endpoint coordinates
if print_all or print_og or print_2d:
    mc_hit_histogram = plt.figure(figsize=(11, 7))
    H2, xedges, yedges, binnmmber = stats.binned_statistic_2d(mc_local_endpoints['x'], mc_local_endpoints['y'], values=pixel_hit['signal'], statistic='mean', bins=[100, 100])
    XX, YY = np.meshgrid(xedges, yedges)
    Hm = ma.masked_where(np.isnan(H2), H2)
    plt.pcolormesh(XX, YY, Hm, cmap="inferno")
    plt.ylabel("pixel y [mm]")
    plt.xlabel("pixel x [mm]")
    cbar = plt.colorbar(pad=.015, aspect=20)
    cbar.set_label("hit signal")
    if save_pdf:  mc_hit_histogram.savefig(path.join(outDir , "GlobalHeatMap.pdf"), bbox_inches='tight')

if print_all or print_og or print_2d:
    mc_hit_histogram_z = plt.figure(figsize=(11, 7))
    H2, xedges, yedges, binnmmber = stats.binned_statistic_2d(mc_local_endpoints['x'], mc_local_endpoints['z'],
                                                              values=pixel_hit['signal'], statistic='mean', bins=[100, 100])
    XX, YY = np.meshgrid(xedges, yedges)
    Hm = ma.masked_where(np.isnan(H2), H2)
    plt.pcolormesh(XX, YY, Hm.T, cmap="inferno")
    plt.ylabel("pixel z [mm]")
    plt.xlabel("pixel x [mm]")
    cbar = plt.colorbar(pad=.015, aspect=20)
    cbar.set_label("hit signal ")
    if save_pdf: mc_hit_histogram_z.savefig(path.join(outDir , "Z_Hist.pdf"), bbox_inches='tight')

if print_all or print_og:
    figMC_z = plt.figure()
    plt.hist(mc_local_endpoints['z'], 100)
    plt.title("MC global end point z coordinate")
    plt.xlabel("z_global [mm]")

    figMC_z_log = plt.figure()
    plt.hist(mc_global_endpoints['x'], 100)
    plt.title("MC global end point z coordinate")
    plt.xlabel("z_global [mm]")
    plt.yscale('log')
    if save_pdf: figMC_z_log.savefig(path.join(outDir , "ZLog_Hist.pdf"), bbox_inches='tight')

# X Pixel Hit
if print_all or print_1d:
    plt.clf()
    figMC_x = plt.figure()
    plt.hist(pixel_hit['x'], len(np.unique(pixel_hit['x'])))
    plt.title("Hit Pixel x coordinate")
    plt.xlabel("hit pixel x coordinate pixels")
    if save_pdf: figMC_x.savefig(path.join(outDir , "XPixelHit.pdf"), bbox_inches='tight')


# X Pixel Hit Gaussian Density
if print_all or print_1d or print_gauss:
    plt.clf()
    x_gauss = plt.figure()
    plt.hist(pixel_hit['x'], len(np.unique(pixel_hit['x'])), density=True)
    plt.xlabel("hit pixel x coordinate pixels")

    mu, sigma = norm.fit(pixel_hit['x'])
    x = np.linspace(min(pixel_hit['x']), max(pixel_hit['x']), len(np.unique(pixel_hit['x'])))
    plt.plot(x, stats.norm.pdf(x, mu, sigma))
    plt.title("Hit Pixel Density \n (mean = " + str(round(mu,2)) + "  sigma = " + str(round(sigma,2)))
    if save_pdf: x_gauss.savefig(path.join(outDir , "XGaussianDist.pdf"), bbox_inches='tight')

# X Pixel Hit Cauchy Density
if print_all or print_1d or print_cauch:
    plt.clf()
    x_cauch = plt.figure()
    plt.hist(pixel_hit['x'], len(np.unique(pixel_hit['x'])), density=True)
    plt.xlabel("hit pixel X coordinate pixels")

    mean, var = stats.cauchy.fit(pixel_hit['x'])
    x = np.linspace(min(pixel_hit['y']), max(pixel_hit['x']), len(np.unique(pixel_hit['x'])))
    plt.plot(x, stats.cauchy.pdf(x, mean, var))
    plt.title("Hit Pixel Density (with Cauchy curve) \n (mean = " + str(round(mean,2)) + "  Variance = " + str(round(var,2)) + ")")
    if save_pdf: x_cauch.savefig(path.join(outDir , "XCauchyDist.pdf"), bbox_inches='tight')



# Y Pixel Hit
if print_all or print_1d:
    plt.clf()
    figMC_x = plt.figure()
    plt.hist(pixel_hit['y'], len(np.unique(pixel_hit['y'])))
    plt.title("Hit Pixel y coordinate")
    plt.xlabel("hit pixel y coordinate pixels")
    if save_pdf: figMC_x.savefig(path.join(outDir , "YPixelHit.pdf"), bbox_inches='tight')

# Y Pixel Hit Gaussian Density
if print_all or print_1d or print_gauss:
    plt.clf()
    y_gauss = plt.figure()
    plt.hist(pixel_hit['y'], len(np.unique(pixel_hit['y'])), density=True)
    plt.xlabel("hit pixel y coordinate pixels")

    mu, sigma = norm.fit(pixel_hit['y'])
    x = np.linspace(min(pixel_hit['y']), max(pixel_hit['y']), len(np.unique(pixel_hit['y'])))
    plt.plot(x, stats.norm.pdf(x, mu, sigma))
    plt.title("Hit Pixel Density\n (mean = " + str(round(mu,2)) + "  sigma = " + str(round(sigma,2)) + ")")
    if save_pdf: y_gauss.savefig(path.join(outDir , "YGaussianDist.pdf"), bbox_inches='tight')


# Y Pixel Hit Cauchy Density
if print_all or print_1d or print_cauch:
    plt.clf()
    y_cauch = plt.figure()
    plt.hist(pixel_hit['y'], len(np.unique(pixel_hit['y'])), density=True)
    plt.xlabel("hit pixel y coordinate pixels")

    mean, var = stats.cauchy.fit(pixel_hit['y'])
    x = np.linspace(min(pixel_hit['y']), max(pixel_hit['y']), len(np.unique(pixel_hit['y'])))
    plt.plot(x, stats.cauchy.pdf(x, mean, var))
    plt.title("Hit Pixel Density (with Cauchy curve) \n (mean = " + str(round(mean,2)) + "  Variance = " + str(round(var,2)) + ")")
    if save_pdf: y_cauch.savefig(path.join(outDir , "YCauchyDist.pdf"), bbox_inches='tight')



# plot pixel hits in pixel Bins?
if print_all or print_2d:
    plt.clf()
    px_bins_hm =plt.figure()
    heatmap, xedges, yedges = np.histogram2d(pixel_hit['x'], pixel_hit['y'], bins=[len(np.unique(pixel_hit['x'])),len(np.unique(pixel_hit['y']))])
    extent = [xedges[0], xedges[-1], yedges[0], yedges[-1]]
    plt.imshow(heatmap.T, origin='lower')
    plt.xlabel('pixel x position')
    plt.ylabel('pixel y position')
    plt.title('Pixel Heatmap (Number of Hits)')
    cbar = plt.colorbar(pad=.015, aspect=20)
    if save_pdf: px_bins_hm.savefig(path.join(outDir , "PixelNumHits.pdf"), bbox_inches='tight', dpi=900)

if print_all or print_1d:
    # Pixel Charge Spectrum (Added)
    plt.clf()
    px_charge_spectrum = plt.figure()
    plt.hist(pixel_hit['signal'], bins=100)
    if save_pdf: px_charge_spectrum.savefig(path.join(outDir , "PixelChargeSpectrum.pdf"), bbox_inches='tight')

# 3D plot x,y,z MC particle endpoint with color scale representing the amount of charge (quite heavy)
if print_all or print_3d or print_og:
    fig3D = plt.figure()
    ax = fig3D.add_subplot(111, projection='3d')
    colors = LinearSegmentedColormap('colormap', cm.jet._segmentdata.copy(), np.max(pixel_hit['signal']))
    plot = ax.scatter(mc_local_endpoints['x'], mc_local_endpoints['y'], mc_local_endpoints['z'], c=pixel_hit['signal'],
                      cmap="jet", marker=".", s=2)  # ,c= marker=m)
    ax.set_ylabel("y mc (mm)")
    ax.set_xlabel("x mc (mm)")
    ax.set_zlabel("z mc (mm)")
    cbar = fig3D.colorbar(plot, ax=ax)
    cbar.set_label("hit signal")
    if save_pdf: fig3D.savefig(path.join(outDir , "3D.pdf"), bbox_inches='tight')
    # show plots

plt.show()
