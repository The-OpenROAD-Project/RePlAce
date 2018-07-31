# RePlAce
*RePlAce: Advancing Solution Quality and Routability Validation in Global Placement* 

| <img src="/doc/image/adaptec2.inf.gif" width=450px> | 
|:--:| 
| *Visualized examples from ISPD 2006 contest; adaptec2.inf* |

### Pre-requisite
* Intel MKL and IPP package

### How To Compile
    $ git clone --recursive https://github.com/abk-openroad/RePlAce.git
    
Then, modify the __MKLROOT__ and __IPPROOT__ to the corresponding install paths in [src/Makefile](src/Makefile)

    $ cd ~/RePlAce
    $ make clean
    $ make 
    
### How To Execute
    // download lefdef benchmarks
    $ cd ~/RePlAce/becnh/lefdef
    $ ./download_ispd18.sh
    
    // download bookshelf benchmarks
    $ cd ~/RePlAce/bench/bookshelf
    $ ./download_dac2012.sh
    
    // Generate a result from ISPD18 - ispd18.test1.input
    // Check doc/ScriptUsage.md in detail
    $ cd ~/RePlAce/src
    $ ./execute_lefdef.py 0 
    
    // Generate a result from DAC2012 - superblue19
    // Check doc/ScriptUsage.md in detail
    $ cd ~/RePlAce/src
    $ ./execute_bookshelf.py superblue19

### Manual
* [doc/ScriptUsage.md](doc/ScriptUsage.md)
* [doc/BinaryArguments.md](doc/BinaryArguments.md)
    
### License
* BSD-3-clause License [[Link]](LICENSE)

### 3rd Party Module List
* Eigen
* CImg
* Google Dense Hash Map
* Ben Marshall's verilog-parser (Modified by mgwoo)
* LEF/DEF Parser (Modified by mgwoo)


### Authors
- Ilgweon Kang and Lutong Wang (respective Ph.D. advisors: Chung-Kuan Cheng, Andrew B. Kahng), based on Dr. Jingwei Lu with ePlace and ePlace-MS
- Many subsequent improvements were made by Mingyu Woo leading up to the initial release.
