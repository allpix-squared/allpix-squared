#!/usr/bin/python3

# SPDX-FileCopyrightText: 2022-2024 CERN and the Allpix Squared authors
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
    pv1 = hm.FourVector(0 * random.random(), 0 * random.random(), 7000 * random.random(), 7000 * random.random())
    p1 = hm.GenParticle(pv1, 2212, 3)
    evt.add_particle(p1)
    v1.add_particle_in(p1)

    v2 = hm.GenVertex()
    evt.add_vertex(v2)
    pv2 = hm.FourVector(0 * random.random(), 0 * random.random(), -7000 * random.random(), 7000 * random.random())
    p2 = hm.GenParticle(pv2, 2212, 3)
    evt.add_particle(p2)
    v2.add_particle_in(p2)

    pv3 = hm.FourVector(0.751 * random.random(), -1.569 * random.random(), 32.191 * random.random(), 32.238 * random.random())
    p3 = hm.GenParticle(pv3, 1, 3)
    evt.add_particle(p3)
    v1.add_particle_out(p3)

    pv4 = hm.FourVector(-3.047 * random.random(), -19.0 * random.random(), -54.629 * random.random(), 57.920 * random.random())
    p4 = hm.GenParticle(pv4, -2, 3)
    evt.add_particle(p4)
    v2.add_particle_out(p4)

    v3 = hm.GenVertex()
    evt.add_vertex(v3)
    v3.add_particle_in(p3)
    v3.add_particle_in(p4)

    pv6 = hm.FourVector(-3.813* random.random(), 0.113* random.random(), -1.833* random.random(), 4.233* random.random())
    p6 = hm.GenParticle(pv6, 22, 1)
    evt.add_particle(p6)
    v3.add_particle_out(p6)

    pv5 = hm.FourVector(1.517* random.random(), -20.68* random.random(), -20.605* random.random(), 85.925* random.random())
    p5 = hm.GenParticle(pv5, -24, 3)
    evt.add_particle(p5)
    v3.add_particle_out(p5)
    # create v4
    v4 = hm.GenVertex(hm.FourVector(0.12* random.random(), -0.3* random.random(), 0.05* random.random(), 0.004* random.random()))
    evt.add_vertex(v4)
    v4.add_particle_in(p5)

    pv7 = hm.FourVector(-2.445 * random.random(), 28.816 * random.random(), 6.082 * random.random(), 29.552 * random.random())
    p7 = hm.GenParticle(pv7, 2212, 1)
    evt.add_particle(p7)
    v4.add_particle_out(p7)

    pv8 = hm.FourVector(3.962 * random.random(), -49.498 * random.random(), -26.687 * random.random(), 560.373 * random.random())
    p8 = hm.GenParticle(pv8, 11, 1)
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
    if args.seed is not None:
        print("Seeding PRNG with seed %i" % args.seed)
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
