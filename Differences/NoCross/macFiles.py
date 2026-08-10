#!/usr/bin/python3

import numpy as np
import os
import shutil

N = 1e7

energies = [1000, 5000, 10000]

print(f'{len(energies)} peaks')

macro="""
# Geometry =====================================================================
/Chamber/Construct
/CeBr3/GeometryFile demonstrator.geo

/Brick/Construct
/CeBr3/GeometryFile bricks.geo

/Source/Simple {en} keV

# Initialize run manager =======================================================
/run/initialize

/Output/Filename e{en}.out
/run/beamOn {n}
"""

for i in range(len(energies)):
    e = f"{energies[i]:.1f}"
    n = f"{N:.0f}"
    macFileName = f"e{e}.mac"
    if not os.path.exists(macFileName):
        macFile = open(macFileName, 'w')
        macFile.write( macro.format(en = e, n = n) )
        macFile.close()
