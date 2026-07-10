#!/usr/bin/python3

import ROOT as root
import numpy as np
from matplotlib import pyplot as plt
import sys
import os

# The detector resolution is simulated as sigmaPars[det]*sqrt(E)
sigmaPars = {0:0.5}

# Histogram specifications
eMax       = 4096
nBins      = 4096

def readInputFile(fileName = "cs137_sim_sort.inp"):

    try:
        inFile = open(fileName, 'r')
    except IOError:
        return
    
    line     = inFile.readline()
    words    = line.split()
    nDet     = int(words[0])

    line     = inFile.readline()
    words    = line.split()
    for det in range(nDet):
        sigmaPars[det] = float(words[det])

    line     = inFile.readline()
    words    = line.split()
    eMax     = float(words[0])

    line     = inFile.readline()
    words    = line.split()
    nBins    = int(words[0])

    inFile.close()

    return nDet, sigmaPars, eMax, nBins 

def Sort(fileName, nDet=1):

    try:
        inFile = open(fileName, 'r')
    except IOError:
        print('Error: unable to open file {0}'.format(fileName))
        return
    
    # Sort the output file.
    FEPCounts = 0
    lastEvent  = -1
    lastDet    = -1
    lastEnergy = 0
    while 1:
        line = inFile.readline()
        if line == "":
            break
        words = line.split()
        if words[0] == 'D':
            NDetsHit = int(words[1])
            event = int(words[2])
            for i in range(NDetsHit):
                line = inFile.readline()
                words = line.split()
                if words[0] == 'C':
                    det   = int(words[1])-1
                    eSim  = float(words[2])
                    if int(words[6]) == 1:
                        FEPCounts += 1
                else:
                    print("Error: Expecting a 'C' entry in output.")
                    exit()
        
                # Fold in simulated resolution.
        
                sigma = sigmaPars[det]*np.sqrt(eSim)
                eRes = eSim + np.random.normal(scale=sigma)

                histosRaw[det].Fill(eSim)
                histos[det].Fill(eRes)        

        if words[0] == 'E':
            NEmittedGammas = int(words[1])
            for i in range(NEmittedGammas):
                line = inFile.readline()

    print('{0:d} Full-energy counts'.format(FEPCounts))

    return 

nDet, sigmaPars, eMax, nBins = readInputFile()

# Create histograms
histos = []
histosRaw = []
for i in range(nDet):
    hRaw = root.TH1F(f"enRaw{i}", f"Detector {i} energy",
                     nBins, 0, eMax)
    histosRaw.append(hRaw)
    h    = root.TH1F(f"en{i}", f"Detector {i} energy",
                     nBins, 0, eMax)
    histos.append(h)

Sort(sys.argv[1], nDet)

# Write histograms

outFileName, _ = os.path.splitext(sys.argv[1])
outFileName += ".root"
outFile = root.TFile(outFileName, "RECREATE")
for i in range(nDet):
    histosRaw[i].Write()
    histos[i].Write()

outFile.Close()
