# RePlAce
RePlAce: Advancing Solution Quality and Routability Validation in Global Placement

If you want to use this as part of the OpenROAD project you should build it and use it from inside the integrated [openroad app](https://github.com/The-OpenROAD-Project/OpenROAD). The standalone version is available as a legacy code in [standalone branch](https://github.com/The-OpenROAD-Project/RePlAce/tree/standalone).

## How to Download/Build?
- For OpenROAD-flow users, manuals for released binaries are available in readthedocs! [(Getting-Started)](https://openroad.readthedocs.io/en/latest/user/getting-started.html)
- For developers, manuals for building a binary is available in OpenROAD repo. [(OpenROAD repo)](https://github.com/The-OpenROAD-Project/OpenROAD) 
- Note that RePlAce is a submodule of OpenROAD repo, and take a place as **"global_placement"** commands. 

## Features
- Analytic and nonlinear placement algorithm. Solves electrostatic force equations using Nesterov's method. ([link](https://cseweb.ucsd.edu/~jlu/papers/eplace-todaes14/paper.pdf))
- Verified with various commercial technologies using OpenDB (7/14/16/28/45/55/65nm).
- Verified deterministic solution generation with various compilers and OS. 
  * Compiler: gcc4.8-9.1/clang-7-9/apple-clang-11
  * OS: Ubuntu 16.04-18.04 / CentOS 6-8 / OSX 
- Cleanly rewritten as C++11.
- Supports Mixed-size placement mode.
- Supports fast image drawing modes with CImg library.

| <img src="/doc/image/adaptec2.inf.gif" width=350px> | <img src="/doc/image/coyote_TSMC16.gif" width=400px> | 
|:--:|:--:|
| *Visualized examples from ISPD 2006 contest; adaptec2.inf* |*Real-world Design: Coyote (TSMC16 7.5T)* |

### Verified/supported Technologies
* ASAP 7
* GF 14
* TSMC 16 (7.5T/9T)
* ST FDSOI 28
* TSMC 45
* Fujitsu 55
* TSMC 65

### Manual
* [RePlAce's TCL Commands List](doc/TclCommands.md)
    
### License
* BSD-3-clause License [[Link]](LICENSE)

### 3rd Party Module List
* CImg

### Authors
- Paper reference: C.-K. Cheng, A. B. Kahng, I. Kang and L. Wang, "RePlAce: Advancing Solution Quality and Routability Validation in Global Placement", to appear in IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, 2018.  (Digital Object Identifier: 10.1109/TCAD.2018.2859220)
- Mingyu Woo rewrites the whole RePlAce with a clean C++11 structure.
- The timing-Driven mode has been implemented by Mingyu Woo (only available in [standalone branch](https://github.com/The-OpenROAD-Project/RePlAce/tree/standalone).)
- Timing-Driven and Routability-Driven mode are ongoing with the clean-code structure (in [openroad branch](https://github.com/The-OpenROAD-Project/RePlAce/tree/openroad).)
