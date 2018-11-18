# RePlAce
*RePlAce: Advancing Solution Quality and Routability Validation in Global Placement* 

| <img src="/doc/image/adaptec2.inf.gif" width=450px> | 
|:--:| 
| *Visualized examples from ISPD 2006 contest; adaptec2.inf* |

### Pre-requisite
* Intel MKL and IPP package [Link](https://software.intel.com/en-us/articles/free-ipsxe-tools-and-libraries) >= 2016.3.210
* GCC compiler and libstdc++ static library >= 5.4.0
* boost library >= 1.53
* X11 library (for CImg library to visualize) >= 1.6.5
* Recommended OS: Centos7 or Ubuntu 16.04

### How To Compile
    $ git clone --recursive https://github.com/abk-openroad/RePlAce.git
    
Then, modify the __MKLROOT__ and __IPPROOT__ to the corresponding install paths in [src/Makefile](src/Makefile)

    $ cd ~/RePlAce
    $ make clean
    $ ./prerequisite/install_centos7.sh   // for centos 7
    $ ./prerequisite/install_ubuntu16.sh  // for ubuntu 16
    $ make 
    
### How To Execute
    // download lefdef benchmarks
    $ cd ~/RePlAce/bench/lefdef
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
* FLUTE
* NTUPlacer3/4h (Thanks for agreeing with redistribution)
* Ben Marshall's verilog-parser (Modified by mgwoo)
* LEF/DEF Parser (Modified by mgwoo)


### Authors
- Ilgweon Kang and Lutong Wang (respective Ph.D. advisors: Chung-Kuan Cheng, Andrew B. Kahng), based on Dr. Jingwei Lu's Fall 2015 code implementing ePlace and ePlace-MS.
- Many subsequent improvements were made by Mingyu Woo leading up to the initial release.
- Paper reference: C.-K. Cheng, A. B. Kahng, I. Kang and L. Wang, "RePlAce: Advancing Solution Quality and Routability Validation in Global Placement", to appear in IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, 2018.  (Digital Object Identifier: 10.1109/TCAD.2018.2859220)

### How to report bugs?
* [doc/ReportBug.md](doc/ReportBug.md) 
