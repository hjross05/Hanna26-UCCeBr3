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

def readInputFile(fileName = "co60_sim_sort.inp"):

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

                # Symmetrized gamma-gamma matrix
                if event == lastEvent and det != lastDet:
                    gam_gam.Fill(eRes, lastEnergy)
                    gam_gam.Fill(lastEnergy, eRes)

                    # 1332 keV cut
                    if eRes > 1260 and eRes < 1400:
                        cut_1332.Fill(lastEnergy)
                    if lastEnergy > 1260 and lastEnergy < 1400:
                        cut_1332.Fill(eRes)
        
                lastEvent = event
                lastDet = det
                lastEnergy = eRes

        if words[0] == 'E' and int(words[1]) == 2:
            # 1332
            line = inFile.readline()
            words = line.split()
            ph1 = float(words[4])
            th1 = float(words[5])
            
            # 1173
            line = inFile.readline()
            words = line.split()
            ph2 = float(words[4])
            th2 = float(words[5])

            v1 = root.TVector3(0,0,1)
            v1.SetTheta(th1)
            v1.SetPhi(ph1)
            v2 = root.TVector3(0,0,1)
            v2.SetTheta(th2)
            v2.SetPhi(ph2)

            dth = np.arccos(v1.Dot(v2)/v1.Mag()/v2.Mag())
            emitted_delta_theta.Fill(np.degrees(dth))
            emitted_theta1.Fill(np.degrees(th1))

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

gam_gam = root.TH2F("gamma_gamma", "Coincidence Matrix",
                    int(nBins/4), 0, eMax,
                    int(nBins/4), 0, eMax)
cut_1332 = root.TH1F("cut_1332", "1332 keV cut", nBins, 0, eMax)

emitted_delta_theta = root.TH1F("emitted_delta_theta", "emitted_delta_theta",
                                180, 0, 180)
emitted_theta1 = root.TH1F("emitted_theta1", "emitted_theta1", 180, 0, 180)
emitted_w      = root.TH1F("emitted_w", "emitted_w", 180, 0, 180)

Sort(sys.argv[1], nDet)

# Write histograms

outFileName, _ = os.path.splitext(sys.argv[1])
outFileName += ".root"
outFile = root.TFile(outFileName, "RECREATE")
for i in range(nDet):
    histosRaw[i].Write()
    histos[i].Write()
gam_gam.Write()
cut_1332.Write()
emitted_delta_theta.Write()
emitted_theta1.Write()

emitted_delta_theta.Sumw2()
emitted_theta1.Sumw2()
emitted_w.Divide(emitted_delta_theta, emitted_theta1)
emitted_w.Write()

outFile.Close()
