#!/usr/bin/python

#################################################################
# This script is written by Mingyu Woo. (16/12/17)
# http://mgwoo.github.io
#################################################################


import os
import sys
import subprocess as sp
from datetime import datetime

#####################################
# configure 
bmflag = "etc"
dpflag = "NTU3"
dploc = "../ntuplace/ntuplace3"
dirpos = "../bench/bookshelf"
binaryName = "./RePlAce"
outpos = "../output"
logpos = "../logdir"

numThread = 1

isOnlyGP = False
isValgrind = False 
isNohup = False
isCentos = False
useScreen = False 
isAplot = False 

## configure done 
#####################################

def ExecuteCommand( command ):
    print( command )
    sp.call( command, shell=True )

def GetAuxFileName( benchFolderPos ):
    lis = os.listdir( benchFolderPos )
    for curFile in lis:
        if curFile.find(".aux") != -1:
            return curFile

    return None

dirList = os.listdir(dirpos)
try:
    dirList.remove('download_dac2012.sh') 
except ValueError:
    pass

if len(sys.argv) <=1:
    print("usage:   ./execute.py <benchname or number>")
    print("Example: ")
    print("         ./execute.py 0" )
    print("         ./execute.py superblue19")
    print("")
    print("Dir List: ")

    for idx, cdir in enumerate(sorted(dirList)):
        print("%d %s" % (idx, cdir))
    sys.exit(1)

benchNum = -1
benchName = ""
if sys.argv[1].isdigit():
    benchNum = int(sys.argv[1])
    benchName = sorted(dirList)[benchNum]
elif sys.argv[1] == "all":
    benchName = sorted(dirList)
else:
    benchName = sys.argv[1]

curTime = datetime.now().strftime('%Y-%m-%d_%H:%M:%S')

placeModeStr = "-onlyGP" if isOnlyGP else "-dpflag %s -dploc %s" % (dpflag, dploc)
directStr = ">" if isNohup else "|& tee" if isCentos else "| tee"
aplotStr = "-aplot" if isAplot else ""

valgrindStr = "valgrind --log-fd=1" if isValgrind else ""
nohupFrontStr = "nohup" if isNohup else "" 
nohupEndStr = "&" if isNohup else ""

addStr = ""
for i in range(2, len(sys.argv)):
    addStr += sys.argv[i] + " "

exeFormat = "screen -S %s -X stuff \"%s %s %s -bmflag %s -aux %s/%s/%s -output %s -t %d %s " \
        + addStr + " %s %s %s/%s_%s.log %s\n\"" if useScreen \
else "%s %s %s -bmflag %s -aux %s/%s/%s -output %s -t %d %s " + addStr + " %s %s %s/%s_%s.log %s"


if type(benchName) is list:
    for curBench in benchName:
        if "__" in curBench:
            continue
        
        print( curBench ) 
        fileName = curBench + "_valgrind" if isValgrind else curBench
        
        AuxName = GetAuxFileName( "%s/%s" % (dirpos, curBench) )
        
        if useScreen:
            ExecuteCommand( "screen -dmS %s bash" % (curBench) )
            ExecuteCommand( exeFormat % (curBench, nohupFrontStr, valgrindStr, \
                    binaryName, bmflag, \
                    dirpos, curBench, AuxName, \
                    outpos, numThread, placeModeStr, aplotStr, \
                    directStr, logpos, fileName, curTime, nohupEndStr ) )
        else:
            ExecuteCommand( exeFormat % (nohupFrontStr, valgrindStr, \
                    binaryName, bmflag, \
                    dirpos, curBench, AuxName, \
                    outpos, numThread, placeModeStr, aplotStr, \
                    directStr, logpos, fileName, curTime, nohupEndStr ) )


else:
    fileName = benchName + "_valgrind" if isValgrind else benchName 
    AuxName = GetAuxFileName( "%s/%s" % (dirpos, benchName) )
   
    if useScreen: 
        ExecuteCommand( "screen -dmS %s bash" % (benchName) )
        ExecuteCommand( exeFormat % (benchName, nohupFrontStr, valgrindStr, \
                binaryName, bmflag, \
                dirpos, benchName, AuxName, \
                outpos, numThread, placeModeStr, aplotStr, \
                directStr, logpos, fileName , curTime, nohupEndStr ) )
    else:
        ExecuteCommand( exeFormat % (nohupFrontStr, valgrindStr, \
                binaryName, bmflag, \
                dirpos, benchName, AuxName, \
                outpos, numThread, placeModeStr, aplotStr, \
                directStr, logpos, fileName , curTime, nohupEndStr ) )

