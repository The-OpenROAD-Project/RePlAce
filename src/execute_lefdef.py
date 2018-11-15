#!/usr/bin/python

#################################################################
# This script is written by Mingyu Woo. (16/12/17)
# http://mgwoo.github.io/
#################################################################


import os
import sys
import subprocess as sp
from datetime import datetime
import time

#####################################
# configure 
bmflag = "etc"
dpflag = "NTU3"
dploc = "../ntuplace/ntuplace3"
dirpos = "../bench/lefdef"
binaryName = "./RePlAce"
outpos = "../output"
logpos = "../logdir"

numThread = 1

isOnlyGP = False
isValgrind = False 
isNohup = False
isCentos = False
useScreen = False 
isPlot = False 

## configure done 
#####################################

def ExecuteCommand( command ):
    print( command )
    sp.call( command, shell=True )


def GetFileName( pos, extension ):
    lis = os.listdir( pos )
    res = []
    for curFile in lis:
        if curFile.endswith("." + extension):
            res.append(curFile)
    return res

def GetFileStr( pos, folder, ext, modeStr ):
    retList = GetFileName("%s/%s"%(pos, folder), ext)
    retStr = ""
    for rStr in retList:
        retStr = retStr + "%s %s/%s/%s " % (modeStr, pos, folder, rStr)
    return retStr

dirList = os.listdir(dirpos)
try:
    dirList.remove('download_ispd18.sh')
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
plotStr = "-plot" if isPlot else ""

valgrindStr = "valgrind --log-fd=1" if isValgrind else ""
nohupFrontStr = "nohup" if isNohup else "" 
nohupEndStr = "&" if isNohup else ""

addStr = ""
for i in range(2, len(sys.argv)):
    addStr += sys.argv[i] + " "

origFormat = "%s %s %s -bmflag %s FILESTR -output %s -t %d %s " + addStr + " %s %s %s/%s_%s.log %s"

if type(benchName) is list:
    for curBench in benchName:
        if "__" in curBench:
            continue

        print( curBench ) 
        fileName = curBench + "_valgrind" if isValgrind else curBench
        
        lefStr = GetFileStr( dirpos, curBench, 'lef', '-lef')
        defStr = GetFileStr( dirpos, curBench, 'def', '-def')
        verilogStr = GetFileStr( dirpos, curBench, 'v', '-verilog')

        exeFormat = origFormat.replace( "FILESTR", lefStr + defStr + verilogStr )
        exeFormat = "screen -S %s -X stuff \"" + exeFormat + "\n\"" if useScreen else exeFormat

        nameList = (nohupFrontStr, valgrindStr, \
                    binaryName, bmflag, \
                    outpos, numThread, placeModeStr,\
                    directStr, logpos, fileName, curTime, nohupEndStr) 
        
        if useScreen:
            ExecuteCommand( "screen -dmS %s bash" % (curBench) )
            ExecuteCommand( exeFormat % (curBench, nohupFrontStr, valgrindStr, \
                    binaryName, bmflag, \
                    outpos, numThread, placeModeStr, plotStr, \
                    directStr, logpos, fileName, curTime, nohupEndStr ) )
        else:
            ExecuteCommand( exeFormat % (nohupFrontStr, valgrindStr, \
                    binaryName, bmflag, \
                    outpos, numThread, placeModeStr, plotStr, \
                    directStr, logpos, fileName, curTime, nohupEndStr ) )


else:
    fileName = benchName + "_valgrind" if isValgrind else benchName 
    
    lefStr = GetFileStr( dirpos, benchName, 'lef', '-lef')
    defStr = GetFileStr( dirpos, benchName, 'def', '-def')
    verilogStr = GetFileStr( dirpos, benchName, 'v', '-verilog')
        
    exeFormat = origFormat.replace( "FILESTR", lefStr + defStr + verilogStr )
    exeFormat = "screen -S %s -X stuff \"" + exeFormat + "\n\"" if useScreen else exeFormat
   
    if useScreen: 
        ExecuteCommand( "screen -dmS %s bash" % (benchName) )
        ExecuteCommand( exeFormat % (benchName, nohupFrontStr, valgrindStr,\
                binaryName, bmflag, \
                outpos, numThread, placeModeStr, plotStr, \
                directStr, logpos, fileName , curTime, nohupEndStr ) )
    else:
        ExecuteCommand( exeFormat % (nohupFrontStr, valgrindStr, \
                binaryName, bmflag, \
                outpos, numThread, placeModeStr, plotStr, \
                directStr, logpos, fileName , curTime, nohupEndStr ) )

