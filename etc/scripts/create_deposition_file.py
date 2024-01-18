#!/usr/bin/python3

# SPDX-FileCopyrightText: 2020-2024 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

import sys
import os
import random
import numpy as np
import argparse

from array import array



eventArr = array('i', [0])
energyArr = array('d', [0])
timeArr = array('d', [0])
positionxArr = array('d', [0])
positionyArr = array('d', [0])
positionzArr = array('d', [0])
pdg_codeArr = array('i', [0])
track_idArr = array('i', [0])
parent_idArr = array('i', [0])


# Class to store deposited charges in
class depositedCharge:

    def __init__(self):
        pass

    def setEventNr(self, eventNr):
        self.eventNr = eventNr
    def setEnergy(self, energy):
        self.energy = energy
    def setTime(self, time):
        self.time = time
    def setPositionx(self, positionx):
        self.positionx = positionx
    def setPositiony(self, positiony):
        self.positiony = positiony
    def setPositionz(self, positionz):
        self.positionz = positionz
    def setDetector(self, detector):
        self.detector = detector
    def setPdg_Code(self, pdg_code):
        self.pdg_code = pdg_code
    def setTrack_Id(self, track_id):
        self.track_id = track_id
    def setParent_Id(self, parent_id):
        self.parent_id = parent_id


    # Required for TTrees
    def fillDepositionArrays(self, omit_time, omit_mcparticle):

        global eventArr, energyArr, timeArr, positionxArr, positionyArr, positionzArr, pdg_codeArr, track_idArr, parent_idArr

        eventArr[0] = self.eventNr
        energyArr[0] = self.energy
        if not omit_time:
            timeArr[0] = self.time
        positionxArr[0] = self.positionx
        positionyArr[0] = self.positiony
        positionzArr[0] = self.positionz
        pdg_codeArr[0] = self.pdg_code
        if not omit_mcparticle:
            track_idArr[0] = self.track_id
            parent_idArr[0] = self.parent_id


    # Required for CSV output
    def getDepositionText(self, omit_time, omit_mcparticle):

        text = str(self.pdg_code) + ", "
        if not omit_time:
            text += str(self.time) + ", "
        text += str(self.energy) + ", "
        text += str(self.positionx) + ", "
        text += str(self.positiony) + ", "
        text += str(self.positionz) + ", "
        text += str(self.detector) + ", "
        if not omit_mcparticle:
            text += str(self.track_id) + ", "
            text += str(self.parent_id)

        text += "\n"

        return text


# Calculation of straight particle trajectories in the sensor
def createParticle(particle, nparticles, nsteps, mix):

    pdgCode = 11

    # For time calculation in ns
    timeOffset = 1.

    # Arbitrary entry and exit points in global coordinates
    entryX = random.gauss(0.5, 0.15)
    exitX = random.gauss(entryX, 0.05)
    entryY = random.gauss(-0.5, 0.15)
    exitY = random.gauss(entryY, 0.05)
    entryPoint = np.array([entryX, entryY, -0.1420])
    exitPoint = np.array([exitX, exitY, 0.1420])

    distVect = exitPoint - entryPoint
    totalDist = np.sqrt(distVect.dot(distVect))

    # Calculate mean energy deposition, 80 e/h pairs per micron are ~200eV/um or 0.2 MeV/mm
    eVPermm = 0.2
    eVPerStep = eVPermm * (totalDist/nsteps)

    # Track parametrization
    zPositions = np.linspace(entryPoint[2],exitPoint[2],nsteps, endpoint=True)
    slope = np.array([(exitPoint[0]-entryPoint[0])/(exitPoint[2]-entryPoint[2]), (exitPoint[1]-entryPoint[1])/(exitPoint[2]-entryPoint[2])])
    offset = np.array([entryPoint[0]-slope[0]*entryPoint[2], entryPoint[1]-slope[1]*entryPoint[2]])

    # Create vector of deposited charges
    deposits = []

    for zPos in zPositions:
        xyPos = offset + slope*zPos
        xyzPos = np.append(xyPos,zPos)

        # Calculate time in ns
        time = timeOffset + zPos*1e-3 / 3e8 * 1e9
        energy = random.gauss(eVPerStep,eVPerStep/30.)

        # Create deposited charge as an object
        deposit = depositedCharge()
        deposit.setEnergy(energy)
        deposit.setTime(time)
        deposit.setPositionx(xyzPos[0])
        deposit.setPositiony(xyzPos[1])
        deposit.setPositionz(xyzPos[2])
        deposit.setPdg_Code(pdgCode)

        # Track and parent ID arbitrary in this case
        if mix:
            deposit.setTrack_Id(particle)
            deposit.setParent_Id(max(0, nparticles - particle))
        else:
            deposit.setTrack_Id(particle)
            deposit.setParent_Id(max(0, particle - 1))

        # Push back vector of deposited charges
        deposits.append(deposit)

    return deposits

def user_input(question):
    if sys.version_info.major == 3:
        return input(question)
    elif sys.version_info.major == 2:
        return raw_input(question)
    else:
        print("Python version could not be determined.")
        exit(1)



if __name__ == '__main__':

    # Check if we have command line control:
    parser = argparse.ArgumentParser()
    parser.add_argument("--type", help="File type to generate, (a) TTree, (b) CSV, (c) both")
    parser.add_argument("--detector", help="Name of the detector")
    parser.add_argument("--outputpath", help="Path to write output files to")
    parser.add_argument("--events", help="Number of events to generate", type=int)
    parser.add_argument("--particles", help="Number of particles to generate for each event", default=1, type=int)
    parser.add_argument("--mix-particles", help="Switch to mix up parent relations to not be sequential", action="store_true")
    parser.add_argument("--seed", help="Seed for random number generator", type=int)
    parser.add_argument("--steps", help="Number of steps along the track in the sensor", type=int)
    parser.add_argument("--scantree", help="Scan generated ROOT tree and print sections", action="store_true")
    parser.add_argument("--omit-time", help="Omit generation of timestamps", action="store_true")
    parser.add_argument("--omit-mcparticle", help="Omit generation of Monte Carlo particles", action="store_true")
    args = parser.parse_args()

    # Seed PRNG with provided seed
    if args.seed is not None:
        random.seed(args.seed)

    # Check for availability of ROOT
    rootAvailable = True
    try:
        import ROOT
    except:
        print("ROOT unavailable. Install ROOT with python option if you are interested in writing TTrees.")
        rootAvailable = False


    # Ask whether to use TTrees or CSV files
    if rootAvailable:
        if args.type is None:
            writeOption = user_input("Generate TTrees (a), a CSV file (b) or both (c)? ")
        else:
            writeOption = args.type

        writeROOT = False
        writeCSV = False
        if writeOption=="a":
            writeROOT = True
        elif writeOption=="b":
            writeCSV = True
        elif writeOption=="c":
            writeROOT = True
            writeCSV = True
        else:
            print("Use one of the three options.")
            exit(1)
    else:
        print("Will just write a CSV file.")
        writeCSV = True
        writeROOT = False


    filenamePrefix = "deposition"
    if args.outputpath:
        filenamePrefix = args.outputpath + "/" + filenamePrefix

    rootfilename = filenamePrefix + ".root"
    csvFilename = filenamePrefix + ".csv"

    # Define detector name
    if args.detector is None:
        detectorName = user_input("Name of your detector: ")
    else:
        detectorName = args.detector

    # Ask for the number of events
    if args.events is None:
        events = int(user_input("Number of events to process: "))
    else:
        events = args.events

    # Ask for the number of steps along the track:
    if args.steps is None:
        nsteps = int(user_input("Number of steps along the track in the sensor: "))
    else:
        nsteps = args.steps

    if writeROOT:
        # Open the file and create the tree
        rootfile = ROOT.TFile(rootfilename,"RECREATE")
        tree = ROOT.TTree("treeName","treeTitle")

        detectorArr = ROOT.vector('string')()
        detectorArr.push_back(detectorName)

        # Create the branches
        eventBranch = tree.Branch("event", eventArr, "event/I")
        energyBranch = tree.Branch("energy", energyArr, "energy/D")
        if not args.omit_time:
            timeBranch = tree.Branch("time", timeArr, "time/D")
        positionxBranch = tree.Branch("position.x", positionxArr, "position.x/D")
        positionyBranch = tree.Branch("position.y", positionyArr, "position.y/D")
        positionzBranch = tree.Branch("position.z", positionzArr, "position.z/D")
        # The char array for the detector branch is a bit more tricky, since you have to give it the length of the name
        detectorBranch = tree.Branch("detector", detectorArr[0], "detector["+str(len(detectorArr[0]))+"]/C")
        pdg_codeBranch = tree.Branch("pdg_code", pdg_codeArr, "pdg_code/I")
        if not args.omit_mcparticle:
            track_idBranch = tree.Branch("track_id", track_idArr, "track_id/I")
            parent_idBranch = tree.Branch("parent_id", parent_idArr, "parent_id/I")


    if writeCSV:
        fout = open(csvFilename,'w')


    for eventNr in range(0,events):
        print("Processing event " + str(eventNr))

        deposits = []
        # Create as many particles as necessary:
        for particle in range(0, args.particles):
            # Get the depositions for the particle created
            deposits = np.append(deposits, createParticle(particle, args.particles, nsteps, args.mix_particles))

        # deposits = createParticle(nsteps);
        if writeCSV:
            # Write the event number
            text = "\nEvent: " + str(eventNr) + "\n"
            fout.write(text)

        for deposit in deposits:
            # Add information to the depositions
            deposit.setEventNr(eventNr)
            deposit.setDetector(detectorName)

            if writeROOT:
                # Fill the arrays and then write them to the tree
                deposit.fillDepositionArrays(args.omit_time, args.omit_mcparticle)
                tree.Fill()

            if writeCSV:
                # extract the text lines for the individual depositions
                text = deposit.getDepositionText(args.omit_time, args.omit_mcparticle)
                fout.write(text)


    if writeROOT:
        # Inspect tree and write ROOT file
        if args.scantree:
            tree.Scan()
        rootfile.Write()
        rootfile.Close()

    if writeCSV:
        # End the file with a line break to prevent from the last line being ignored due to the EOF
        fout.write("\n")
        fout.close()
