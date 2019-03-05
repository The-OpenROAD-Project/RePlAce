# RePlAce script Usage
There is a python script to feed lef/def/verilog files.

* [execute_lefdef.py](../src/execute_lefdef.py)

## Configure options to execute scripts
You can easily find this part inside of the python scripts.

* __bmflag__ : (__mms__/__ispd__/__sb__/__ibm__/__etc__) Benchmark flag. Default is 'etc'
* __dpflag__ : (__NTU3__/__NTU4__) Detail placer flag. Current RePlAce can take 'NTU3' or 'NTU4'
* __dploc__ : (__string__) Detail placer binary location
* __dirpos__ : (__string__) Benchmark directory position
* __binaryName__ : (__string__) Compiled RePlAce binary name
* __outpos__ : (__string__) Set a output folder location - The binary saves 'jpg/def/bookshelf' files
* __logpos__ : (__string__) Set a log folder location - The binary saves standard-output log like superblue9_2018-07-30_20:46:56.log
* __numThread__ : (__int__) Define the number of threads RePlAce can use
* __isOnlyGP__ : (__T__/__F__) Determine whether RePlAce stops after Global placement
* __isPlot__ : (__T__/__F__) Determine whether RePlAce generates visualized images (using CImg)
* __isValgrind__ : (__T__/__F__) Use Valgrind to debug
* __isNohup__ : (__T__/__F__) Execute RePlAce in background mode using nohup
* __useScreen__ : (__T__/__F__) Execute RePlAce in background mode using screen

\* Note that __isNohup__ and __useScreen__ should not be __True__ at the same time.
    
## Feed RePlAce with LEF/DEF/VERILOG
    $ python execute_lefdef.py [directory order | benchmark Name | all]

For example, if you have LEF/DEF/verilog benchmarks as

    ReplaceBench/CORTEXM0DS/floorplan.def
    ReplaceBench/CORTEXM0DS/techlib.lef
    ReplaceBench/leon2/leon2.def
    ReplaceBench/leon2/techlib.lef
    ReplaceBench/leon2/leon2.v
    ReplaceBench/superblue19/superblue19.def
    ReplaceBench/superblue19/superblue19.lef

1. Directory order  

        $ python execute_lefdef.py 0
    
    User can specify the sub-directory index (in alphabetical order)
    (0 -> CORTEXM0DS // 1 -> leon2 // 2 -> superblue19)

2. Directory Name  

        $ python execute_lefdef.py CORTEXM0DS
    
    User can also specify the exact name of a sub-folder name  
    (CORTEXM0DS -> CORTEXM0DS // leon2 -> leon2 // superblue19 -> superblue19)

3. All directory simultaneously (+ background)  

        $ python execute_lefdef.py all
        
    Then, it can execute all files in sequentially. 
    If you set __useScreen__ = __True__ or __nohup__ = __True__, all benchmarks are executed simultaneously as background processes.


## Notice

1. The name of DEF/LEF/verilog need not to be same as the folder name.  
    It automatically detects whether the '\*.lef/\*.def/\*.v' files exist or not. However, the name of 'output' folder will be set as 'DEF' name. ( superblue19/__floorplan_good__.def -> {outpos}/{bmflag}/__floorplan_good__/__floorplan_good__\_placed.def )

2. Verilog is optional to feed RePlAce.  
    If netlist is found in both of verilog and DEF, RePlAce shows __WARNING__, and parses netlist from the DEF file.  
    (DEF > Verilog)

