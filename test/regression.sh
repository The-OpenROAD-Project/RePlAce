#!/bin/bash

source function.sh

BINARY_LOC="./replace"
s=0
t=0

# Timing-Driven test for A2A 
# RunTest binary      tech        design      density   bin     SkipIP
RunTest   $BINARY_LOC a2a         wb_dma_top  1.0       64      true
RunTest   $BINARY_LOC a2a         wb_dma_top  0.9       64      true
RunTest   $BINARY_LOC a2a         wb_dma_top  0.7       64      true

# Timing-Driven test for Nangate45 
RunTest   $BINARY_LOC nangate45   gcd         1.0 
RunTest   $BINARY_LOC nangate45   gcd         0.9
RunTest   $BINARY_LOC nangate45   gcd         0.7
RunTest   $BINARY_LOC nangate45   gcd         0.5

#RunTest  $BINARY_LOC nangate45  dynamic_node_top_wrap 
#RunTest  $BINARY_LOC nangate45  ibex_core 
#RunTest  $BINARY_LOC nangate45  aes_cipher_top 

printf "${GREEN}Success: ${s} / Total: ${t}${NC}"
