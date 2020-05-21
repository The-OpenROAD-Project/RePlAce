# RePlAce
RePlAce: Advancing Solution Quality and Routability Validation in Global Placement
## Features
- Analytic and nonlinear placement algorithm. Solves electrostatic force equations using Nesterov's method. ([link](https://cseweb.ucsd.edu/~jlu/papers/eplace-todaes14/paper.pdf))
- Fully supports commercial formats. (LEF/DEF 5.8)
- Verified and worked well with various commercial technologies. (7/14/16/28/45/55/65nm)
- Supports timing-driven placement mode based on commercial timer (OpenSTA).
- Fast image drawing engine is ported (CImg).
- __RePlAce has TCL interpreter now!__

| <img src="/doc/image/adaptec2.inf.gif" width=350px> | <img src="/doc/image/coyote_TSMC16.gif" width=400px> | 
|:--:|:--:|
| *Visualized examples from ISPD 2006 contest; adaptec2.inf* |*Real-world Design: Coyote (TSMC16 7.5T)* |


# Getting Started

## Run using Docker
1. Install Docker on [Windows](https://docs.docker.com/docker-for-windows/), [Mac](https://docs.docker.com/docker-for-mac/) or [Linux](https://docs.docker.com/install/).
2. Navigate to the directory where you have the input files.
3. Run RePlAce container:

       docker run -it -v $(pwd):/data openroad/replace bash

4. From the interactive bash terminal, use RePlAce scripts which reside under `/RePlAce`. You can read input files from `/data` directory inside the docker container - which mirrors the host machine directory you are in. 

* The Docker image is self-contained and includes everything that RePlAce needs to work properly.

## Install on a bare-metal machine

### Pre-requisite
* GCC compiler and libstdc++ static library >= 4.8.5
* boost library >= 1.41
* bison (for lef/def parsers) >= 3.0.4
* tcl >= 8.4
* X11 library (for CImg library to visualize) >= 1.6.5
* Recommended OS: Centos6, Centos7 or Ubuntu 16.04

### Clone repo and submodules 
    $ git clone --recursive --branch standalone https://github.com/The-OpenROAD-Project/RePlAce.git
    $ cd ~/RePlAce
    $ ./prerequisite/install_centos7.sh   // for centos 7
    $ ./prerequisite/install_ubuntu16.sh  // for ubuntu 16
    $ mkdir build
    $ cd build
    $ cmake ..                            // you may specify -DCMAKE_INSTALL_PREFIX to avoid installing in /usr/local/*
    $ make 
    $ make install                        // if you want to install

   
### Check your installation
    To make sure your installation is correct and the current tool version is stable enough, 
    run a Hello World application:

    $ cd RePlAce/test
    $ replace < gcd_nontd_test.tcl
    $ replace < gcd_td_test.tcl
 
### How To Execute
| <img src="/doc/image/replace_tcl_interp_example.gif" width=900px> |
|:--:|
| *Example usages with TCL interpreter* |


    // Tcl Interpreter Mode
    // The following command will create a TCL interpreter session.
    $ replace
    
    // The following command will send all TCL commands to RePlAce's TCL interpreter
    $ replace < run_replace.tcl

### Verified/supported Technologies
* TSMC 65
* Fujitsu 55
* TSMC 45
* ST FDSOI 28
* TSMC 16 (7.5T/9T)
* GF 14
* ASAP 7

### Manual
* [RePlAce's TCL Commands List](doc/TclCommands.md)
* [How To Report Memory Bugs?](doc/ReportMemoryBug.md)
    
### License
* BSD-3-clause License [[Link]](LICENSE)
* Code found under the Modules directory (e.g., submodules) have individual copyright and license declarations.

### 3rd Party Module List
* Eigen
* CImg
* [FLUTE](https://github.com/RsynTeam/rsyn-x/tree/master/rsyn/src/rsyn/3rdparty/flute) from [Rsyn-x](https://github.com/RsynTeam/rsyn-x)
* [OpenSTA](https://github.com/The-OpenROAD-Project/OpenSTA)
* NTUPlacer3/4h (Thanks for agreeing with redistribution)
* LEF/DEF Si2 Parser (Modified by mgwoo)


### Authors
- Ilgweon Kang and Lutong Wang (respective Ph.D. advisors: Chung-Kuan Cheng, Andrew B. Kahng), based on Dr. Jingwei Lu's Fall 2015 code implementing ePlace and ePlace-MS.
- Many subsequent improvements were made by Mingyu Woo leading up to the initial release.
- Paper reference: C.-K. Cheng, A. B. Kahng, I. Kang and L. Wang, "RePlAce: Advancing Solution Quality and Routability Validation in Global Placement", to appear in IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, 2018.  (Digital Object Identifier: 10.1109/TCAD.2018.2859220)
- Timing-Driven mode has been implemented by Mingyu Woo.
- Tcl-Interpreter has been ported by Mingyu Woo.

### Limitations
* Mixed-sized RePlAce with (LEF/DEF/Verilog) interface does not generate legalized placement.
* RePlAce does not support rectilinear layout regions.
