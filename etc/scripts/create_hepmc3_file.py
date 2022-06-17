# SPDX-FileCopyrightText: 2022 CERN and the Allpix Squared authors
# SPDX-License-Identifier: MIT

# Generate some example HepMC3 files in ROOT Tree or ASCII format
# The events generated are not necessarily phyically meaningful but serve as example for generating and
# parsing of event files produced by MC Generators.

import sys
import os
import random
import math
import argparse
from pyHepMC3 import HepMC3 as hm

def generate_HEPEVT(event_id):

    # Build the graph, which will look like
    # Please note this is not physically meaningful event.
    #                       p7                   #
    # p1                   /                     #
    #   \v1__p3      p5---v4                     #
    #         \_v3_/       \                     #
    #         /    \        p8                   #
    #    v2__p4     \                            #
    #   /            p6                          #
    # p2                                         #

    # Create the event container with event number
    evt = hm.GenEvent(hm.Units.GEV, hm.Units.MM)
    evt.set_event_number(event_id)

    v1 = hm.GenVertex()
    evt.add_vertex(v1)
    p1 = hm.GenParticle(hm.FourVector(0, 0, 7000, 7000), 2212, 3)
    evt.add_particle(p1)
    p1.add_attribute("flow1", hm.IntAttribute(231))
    p1.add_attribute("flow1", hm.IntAttribute(231))
    p1.add_attribute("theta", hm.DoubleAttribute(random.random() * math.pi))
    p1.add_attribute("phi", hm.DoubleAttribute(random.random() * math.pi * 2))
    v1.add_particle_in(p1)

    v2 = hm.GenVertex()
    evt.add_vertex(v2)
    p2 = hm.GenParticle(hm.FourVector(0, 0, -7000, 7000), 2212, 3)
    evt.add_particle(p2)
    p2.add_attribute("flow1", hm.IntAttribute(243))
    p2.add_attribute("theta", hm.DoubleAttribute(random.random() * math.pi))
    p2.add_attribute("phi", hm.DoubleAttribute(random.random() * math.pi * 2))
    v2.add_particle_in(p2)

    p3 = hm.GenParticle(hm.FourVector(0.751, -1.569, 32.191, 32.238), 1, 3)
    evt.add_particle(p3)
    p3.add_attribute("flow1", hm.IntAttribute(231))
    p3.add_attribute("theta", hm.DoubleAttribute(random.random() * math.pi))
    p3.add_attribute("phi", hm.DoubleAttribute(random.random() * math.pi * 2))
    v1.add_particle_out(p3)
    p4 = hm.GenParticle(hm.FourVector(-3.047, -19.0, -54.629, 57.920), -2, 3)
    evt.add_particle(p4)
    p4.add_attribute("flow1", hm.IntAttribute(243))
    p4.add_attribute("theta", hm.DoubleAttribute(random.random() * math.pi))
    p4.add_attribute("phi", hm.DoubleAttribute(random.random() * math.pi * 2))
    v2.add_particle_out(p4)

    v3 = hm.GenVertex()
    evt.add_vertex(v3)
    v3.add_particle_in(p3)
    v3.add_particle_in(p4)
    p6 = hm.GenParticle(hm.FourVector(-3.813, 0.113, -1.833, 4.233), 22, 1)
    evt.add_particle(p6)
    p6.add_attribute("flow1", hm.IntAttribute(231))
    p6.add_attribute("theta", hm.DoubleAttribute(random.random() * math.pi))
    p6.add_attribute("phi", hm.DoubleAttribute(random.random() * math.pi * 2))
    v3.add_particle_out(p6)
    p5 = hm.GenParticle(hm.FourVector(1.517, -20.68, -20.605, 85.925), -24, 3)
    evt.add_particle(p5)
    p5.add_attribute("flow1", hm.IntAttribute(243))
    p5.add_attribute("theta", hm.DoubleAttribute(random.random() * math.pi))
    p5.add_attribute("phi", hm.DoubleAttribute(random.random() * math.pi * 2))
    v3.add_particle_out(p5)
    # create v4
    v4 = hm.GenVertex(hm.FourVector(0.12, -0.3, 0.05, 0.004))
    evt.add_vertex(v4)
    v4.add_particle_in(p5)
    p7 = hm.GenParticle(hm.FourVector(-2.445, 28.816, 6.082, 29.552), 2212, 1)
    evt.add_particle(p7)
    v4.add_particle_out(p7)
    p8 = hm.GenParticle(hm.FourVector(3.962, -49.498, -26.687, 560.373), 11, 1)
    evt.add_particle(p8)
    v4.add_particle_out(p8)

    evt.add_attribute("signal_process_vertex", hm.IntAttribute(v3.id()))
    evt.set_beam_particles(p1,p2)

    return evt


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
    parser.add_argument("--events", help="Number of events to generate", type=int)
    parser.add_argument("--outputpath", help="Path to write output files to")
    parser.add_argument("--seed", help="Seed for random number generator", type=int)
    args = parser.parse_args()

    # Seed PRNG with provided seed
    if args.type is not None:
        random.seed(args.seed)

    # Check for availability of ROOT
    rootAvailable = True
    try:
        from pyHepMC3.rootIO import HepMC3 as hmrootIO
    except:
        print("ROOT unavailable. Install ROOT with python option if you are interested in writing TTrees.")
        rootAvailable = False


    # Ask whether to use TTrees or CSV files
    if rootAvailable:
        if args.type is None:
            writeOption = user_input("Generate TTrees (a), a ASCII file (b) or both (c)? ")
        else:
            writeOption = args.type

        writeROOT = False
        writeASCII = False
        if writeOption=="a":
            writeROOT = True
        elif writeOption=="b":
            writeASCII = True
        elif writeOption=="c":
            writeROOT = True
            writeASCII = True
        else:
            print("Use one of the three options.")
            exit(1)
    else:
        print("Will just write a ASCII file.")
        writeASCII = True
        writeROOT = False

    # Ask for the number of events
    if args.events is None:
        events = int(user_input("Number of events to process: "))
    else:
        events = args.events

    filenamePrefix = "hepmc3"
    if args.outputpath:
        filenamePrefix = args.outputpath + "/" + filenamePrefix

    rootfilename = filenamePrefix + ".root"
    asciiFilename = filenamePrefix + ".txt"

    if writeROOT:
        outputROOT = hmrootIO.WriterRootTree(rootfilename)

    if writeASCII:
        outputASCII = hm.WriterAscii(asciiFilename)

    for eventNr in range(0,events):
        print("Processing event " + str(eventNr))
        evt = generate_HEPEVT(eventNr)

        if writeROOT:
            outputROOT.write_event(evt)
        if writeASCII:
            outputASCII.write_event(evt)


    if writeROOT:
        outputROOT.close()
    if writeASCII:
        outputASCII.close()
