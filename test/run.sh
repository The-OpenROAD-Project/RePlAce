#!/bin/bash

get_abs_filename() {
  # $1 : relative filename
  echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
}

DETAIL_PLACER_FLAG="NTU3"
LEF="a2a_wb_dma_top/contest.lef"
DEF="a2a_wb_dma_top/wb_dma_top_soce.def"
VERILOG="a2a_wb_dma_top/wb_dma_top_soce.v"
OUT="out"
DETAIL_PLACE_LOC=$(get_abs_filename "/usr/local/bin/ntuplace3")

RePlAce -bmflag etc \
  -lef $LEF -def $DEF -verilog $VERILOG \
  -output $OUT \
  -dpflag $DETAIL_PLACER_FLAG -dploc $DETAIL_PLACE_LOC \
  -skipIP -bin 64 -plot
