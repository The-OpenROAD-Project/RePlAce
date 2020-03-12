# RePlAce
RePlAce: Advancing Solution Quality and Routability Validation in Global Placement
## Features
- Analytic and nonlinear placement algorithm. Solves electrostatic force equations using Nesterov's method. ([link](https://cseweb.ucsd.edu/~jlu/papers/eplace-todaes14/paper.pdf))
- Verified and worked well with various commercial technologies based on OpenDB (7/14/16/28/45/55/65nm).
- Cleanly rewritten as C++11. 
- Supports Mixed-size placement mode.
- Supports fast image drawing modes with CImg library.

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
    
### License
* BSD-3-clause License [[Link]](LICENSE)

### 3rd Party Module List
* CImg

### Authors
- Paper reference: C.-K. Cheng, A. B. Kahng, I. Kang and L. Wang, "RePlAce: Advancing Solution Quality and Routability Validation in Global Placement", to appear in IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, 2018.  (Digital Object Identifier: 10.1109/TCAD.2018.2859220)
- Mingyu Woo rewrite the whole RePlAce with clean C++ structure.
- Timing-Driven mode has been implemented by Mingyu Woo.
- Tcl-Interpreter has been ported by Mingyu Woo.

### Limitations
* Mixed-sized RePlAce with (LEF/DEF/Verilog) interface does not generate legalized placement.
* RePlAce does not support rectilinear layout regions.
