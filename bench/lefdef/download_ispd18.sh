#!/bin/sh
rm -rf *.tgz 2> /dev/null
for num in {1..10} 
do
    fname=ispd18_test${num}
    wget http://www.ispd.cc/contests/18/${fname}.tgz
    mkdir -p ${fname}
    tar zxvf ${fname}.tgz -C ./$fname/
    rm -rf ${fname}.tgz 2> /dev/null
done
