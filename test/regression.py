import os
import sys
import subprocess as sp
import shlex

useValgrind = False 
useScreen = False 

def ExecuteCommand( cmd, log="" ):
  # print( "CMD: " + cmd )
  # print( shlex.split(cmd) )
  if log == "":
    sp.call( shlex.split(cmd), shell=False, stdout=None )
  else:
    # print( "LOG: " + log )
    f = open(log, "w")
    p = sp.Popen( shlex.split(cmd), stdout=f, stderr=f)
    p.wait()
    f.close()

    f = open(log, "r")
    print( f.read() )
    f.close()

# threshold setting
def DiffVar(gVar, tVar, threshold):
  # smaller is the better
  if gVar >= tVar:
    return True

  if abs(gVar - tVar) / abs(gVar) <= threshold:
    return True
  else:
    return False

def TdGoldenCompare(orig, ok):
  f = open(orig, "r")
  origCont = f.read().split("\n")
  f.close()

  o = open(ok, "r")
  goldenCont = o.read().split("\n")
  o.close()

  gHpwl = float(goldenCont[0].split(": ")[-1])
  gWns = float(goldenCont[1].split(": ")[-1])
  gTns = float(goldenCont[2].split(": ")[-1])

  tHpwl = float(origCont[0].split(": ")[-1])
  tWns = float(origCont[1].split(": ")[-1])
  tTns = float(origCont[2].split(": ")[-1])
 
  if DiffVar(gHpwl, tHpwl, 5) == False:
    print("HPWL has more than 5 percents diff: %g %g" %(gHpwl, tHpwl))
    sys.exit(1)

  if DiffVar(gWns, tWns, 30) == False:
    print("WNS has more than 30 percents diff: %g %g" %(gWns, tWns))
    sys.exit(1)

  if DiffVar(gTns, tTns, 30) == False:
    print("TNS has more than 30 percents diff: %g %g" %(gTns, tTns))
    sys.exit(1)

  print("  " + ok + " passed!")


def TdRun(tdList):
  # regression for TD test cases
  for curTdCase in tdList:
    ExecuteCommand("rm -rf %s/*.rpt" % (curTdCase))
    ExecuteCommand("rm -rf %s/*.log" % (curTdCase))
  
  for curTdCase in tdList:
    print ( "Access " + curTdCase + ":")
    for cFile in os.listdir(curTdCase):
      if cFile.endswith(".tcl") == False:
        continue
      print ( "  " + cFile )
      cmd = "./replace %s/%s" % (curTdCase, cFile)
      log = "%s/%s.log" % (curTdCase, cFile)
      # ExecuteCommand("ln -s *.dat ./%s/" % (curTdCase))
      if useValgrind:
        cmd = "valgrind --log-fd=1 ./replace %s/%s" % (curTdCase, cFile)
        log = "%s/%s_mem_check.log" % (curTdCase, cFile)
        ExecuteCommand(cmd, log)
      elif useScreen:
        scName = "%s_%s" %(curTdCase, cFile)
        ExecuteCommand("screen -dmS %s bash" %(scName))
        ExecuteCommand("screen -S %s -X stuff \"%s \n\"" % (scName, cmd))
      else:
        ExecuteCommand(cmd, log)

    print("Compare with golden: ")
    for cFile in os.listdir(curTdCase):
      if cFile.endswith(".rpt") == False:
        continue
      rptFile = "%s/%s" % (curTdCase, cFile)
      goldenFile = rptFile + ".ok" 
      TdGoldenCompare(rptFile, goldenFile)

    print("")

if len(sys.argv) <= 1:
  print("Usage: python regression.py run")
  print("Usage: python regression.py get")
  sys.exit(0)

dirList = os.listdir(".")
tdList = []
for cdir in sorted(dirList):
  if os.path.isdir(cdir) == False:
    continue

  if "td-test" in cdir:
    tdList.append(cdir)

if sys.argv[1] == "run":
  TdRun(tdList)
elif sys.argv[1] == "get":
  ExecuteCommand("watch -n 3 \"grep -r 'HPWL' td-test*/*.rpt; grep -r 'WNS' td-test*/*.rpt; grep -r 'TNS' td-test*/*.rpt\"")
