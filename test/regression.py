import os
import sys
import subprocess as sp

useValgrind = False 
useScreen = True

def ExecuteCommand( cmd ):
  print( cmd )
  sp.call( cmd, shell=True )

def TdRun(tdList):
  # regression for TD test cases
  for curTdCase in tdList:
    ExecuteCommand("rm -rf %s/exp" % (curTdCase))
    ExecuteCommand("rm -rf %s/output" % (curTdCase))
  
  for curTdCase in tdList:
    print ( "Access " + curTdCase + ":")
    for cFile in os.listdir(curTdCase):
      if cFile.endswith(".tcl") == False:
        continue
      print ( "  " + cFile )
      cmd = "cd %s && ../replace < %s |& tee exp/%s.log" % (curTdCase, cFile, cFile) 
      ExecuteCommand("cd %s && ln -s ../*.dat ./" % (curTdCase))
      if useValgrind: 
        ExecuteCommand("cd %s && valgrind --log-fd=1 ../replace < %s |& tee exp/%s_valgrind.log" % (curTdCase, cFile, cFile) )
      elif useScreen:
        scName = "%s_%s" %(curTdCase, cFile)
        ExecuteCommand("screen -dmS %s bash" %(scName))
        ExecuteCommand("screen -S %s -X stuff \"%s \n\"" % (scName, cmd))
      else:
        ExecuteCommand(cmd)


if len(sys.argv) <= 1:
  print("Usage: python regression.py run")
  print("Usage: python regression.py skill")
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
elif sys.argv[1] == "skill":
  ExecuteCommand("for scr in $(screen -ls | awk '{print $1}'); do screen -S $scr -X kill; done")
else:
  ExecuteCommand("watch -n 3 \"grep -r 'HPWL' td-test*/exp/*.rpt; grep -r 'WNS' td-test*/exp/*.rpt; grep -r 'TNS' td-test*/exp/*.rpt\"")
