# RePlAce
RePlAce: Advancing Solution Quality and Routability Validation in Global Placement
## Features
- Analytic and nonlinear placement algorithm. Solves electrostatic force equations using Nesterov's method. ([link](https://cseweb.ucsd.edu/~jlu/papers/eplace-todaes14/paper.pdf))
- Fully supports commercial formats. (LEF/DEF 5.8)
- Verified and worked well with various commercial technologies. (7/14/16/28/45/55/65nm)
- Supports timing-driven placement mode based on commercial timer (OpenSTA).
- Fast image drawing engine is ported (CImg).

| <img src="/doc/image/adaptec2.inf.gif" width=350px> | <img src="/doc/image/coyote_TSMC16.gif" width=400px> | 
|:--:|:--:|
| *Visualized examples from ISPD 2006 contest; adaptec2.inf* |*Real-world Design: Coyote (TSMC16 7.5T)* |


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
* [FLUTE](https://github.com/RsynTeam/rsyn-x/tree/master/rsyn/src/rsyn/3rdparty/flute) fro* [OpenSTA](https://github.com/The-OpenROAD-Project/OpenSTA)

### Authors
- Ilgweon Kang and Lutong Wang (respective Ph.D. advisors: Chung-Kuan Cheng, Andrew B. Kahng), based on Dr. Jingwei Lu's Fall 2015 code implementing ePlace and ePlace-MS.
- Many subsequent improvements were made by Mingyu Woo leading up to the initial release.
- Paper reference: C.-K. Cheng, A. B. Kahng, I. Kang and L. Wang, "RePlAce: Advancing Solution Quality and Routability Validation in Global Placement", to appear in IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, 2018.  (Digital Object Identifier: 10.1109/TCAD.2018.2859220)
- Timing-Driven mode has been implemented by Mingyu Woo.
- Tcl-Interpreter has been ported by Mingyu Woo.

### Limitations
* Mixed-sized RePlAce with (LEF/DEF/Verilog) interface does not generate legalized placement.
* RePlAce does not support rectilinear layout regions.
