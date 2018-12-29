#!/bin/sh
mkdir -p tar
for num in 2 3 6 7 9 11 12 14 16 19
do
    fname=superblue${num}
    wget http://archive.sigda.org/dac2012/contest/DAC2012_Benchmarks/${fname}.tar.bz2
    tar xvf ${fname}.tar.bz2
done
mv *.tar tar/
