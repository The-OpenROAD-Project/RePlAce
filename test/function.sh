#!/bin/bash

# text coloring
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# lef location return
function GetLef {
  if [ $1 == "a2a" ]; then
    eval "$2='library/a2a/contest.lef'"
  elif [ $1 == "nangate45" ]; then
    eval "$2='library/nangate45/NangateOpenCellLibrary.lef'"
  fi
}

# lib location return
function GetLib {
  if [ $1 == "a2a" ]; then
    eval "$2='library/a2a/contest.lib'"
  elif [ $1 == "nangate45" ]; then
    eval "$2='library/nangate45/NangateOpenCellLibrary_typical.lib'"
  fi
}

# floating arth calculation
function calc { 
  awk "BEGIN { print "$*" }"; 
}

# Simple Test function
# 1 = binaryLoc, 2 = tech, 3 = design
# 4 = successCnt, 5 = totalCnt
function RunTest {
  RunReplace $1 $2 $3 $4 $5 $6
  CompareResult $2 $3 $4
}

# Running function for replace
function RunReplace {
  BINARY_LOC=$1
  TECH=$2
  DESIGN=$3
  DENSITY=$4
  
  GetLef $TECH LEF 
  GetLib $TECH LIB

  DEF="design/${TECH}/${DESIGN}/${DESIGN}.def"
  SDC="design/${TECH}/${DESIGN}/${DESIGN}.sdc"
  VERILOG="design/${TECH}/${DESIGN}/${DESIGN}.v"
  OUT="output/${TECH}_${DESIGN}"

  # $BINARY_LOC -bmflag etc \
  #   -lef $LEF -def $DEF -verilog $VERILOG -lib $LIB -sdc $SDC \
  #   -output $OUT \
  #   -onlyGP -den 0.6 -plot -timing -capPerMicron 0.23e-15 -resPerMicron 23 \
  #   | tee ${DESIGN}_test.log

  args=( -bmflag etc )
  args+=( -lef $LEF )
  args+=( -def $DEF )
  args+=( -output $OUT )
  args+=( -onlyGP -plot )
  
  args+=( -den $DENSITY )
  
  if [ -n "$5" ]; then
    args+=( -bin $5 )
  fi
  if [ -n "$6" ]; then
    args+=( -skipIP )
  fi

  args+=( -timing )
  args+=( -verilog $VERILOG )
  args+=( -lib $LIB )
  args+=( -sdc $SDC )
  args+=( -capPerMicron 0.23e-15 -resPerMicron 23 )

  echo "$2 $3 testing..."
  echo $BINARY_LOC "${args[@]}" 

  $BINARY_LOC "${args[@]}" > ${TECH}_${DESIGN}_${DENSITY}_test.log 2>&1
  #valgrind --log-fd=1 $BINARY_LOC "${args[@]}" | tee ${TECH}_${DESIGN}_valgrind.log 
  
  echo "Done"
  echo ""
}

function CompareResult {
  TECH=$1
  DESIGN=$2
  DENSITY=$3
  OK_HPWL=$(eval cat ok/${TECH}_${DESIGN}_${DENSITY}.log | grep HPWL | tail -n 2 | head -n 1 | cut -d ' ' -f 6)
  CUR_HPWL=$(eval cat ${TECH}_${DESIGN}_${DENSITY}_test.log | grep HPWL | tail -n 2 | head -n 1 | cut -d ' ' -f 6)

  echo "Orignal HPWL: $OK_HPWL"
  echo "New     HPWL: $CUR_HPWL"

  
  RATIO=$(eval calc $OK_HPWL/$CUR_HPWL)
  echo "        Ratio: $RATIO"


  if (( $(echo "$RATIO > 0.95" | bc -l) )) && (( $(echo "$RATIO < 1.05" | bc -l) )); then
    printf "${GREEN}${TECH}_${DESIGN} Success!${NC}"
    s=$((s+1))
  else
    printf "${RED}${TECH}_${DESIGN} Failed!${NC}"
  fi

  t=$((t+1))
  printf '\n'
}
    
