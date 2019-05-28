#!/bin/bash

get_abs_filename() {
  # $1 : relative filename
  echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
}

DETAIL_PLACER_FLAG="NTU4"
LEF="a2a_wb_dma_top/contest.lef"
DEF="a2a_wb_dma_top/wb_dma_top_soce.def"
LIB="a2a_wb_dma_top/contest.lib"
SDC="a2a_wb_dma_top/wb_dma_top.sdc"
VERILOG="a2a_wb_dma_top/wb_dma_top_soce.v"
OUT="a2a_wb_dma_top_out"
DETAIL_PLACE_LOC=$(get_abs_filename "../ntuplace/ntuplace4h")

./replace -bmflag etc \
  -lef $LEF -def $DEF -verilog $VERILOG -lib $LIB -sdc $SDC \
  -output $OUT \
  -dpflag $DETAIL_PLACER_FLAG -dploc $DETAIL_PLACE_LOC \
  -skipIP -bin 64 -plot -timing -capPerMicron 0.23e-15 -resPerMicron 23
