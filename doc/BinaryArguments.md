# Usage

## Lef/Def/Verilog (Commercial Format)
    $ ./RePlAce -bmflag <mms/ispd/sb/ibm/etc> -lef tech.lef -lef macro.lef ...  -def <*.def> [-verilog <*.v>] -output <outputLocation> -dpflag <NTU3/NTU4> -dploc <dpLocation> [Options]

* __-bmflag__ : Specify which Benchmark is used
* __-lef__ : \*.lef Location (Multiple lef files supported. __Technology LEF must be ahead of other LEFs.__)
* __-def__ : \*.def Location (Required due to FloorPlan information)
* __-verilog__ : \*.v Location (Optional)
* __-output__ : Specify the Location of Output Results
* __-dpflag__ : Specify which Detailed Placer is Used
* __-dploc__ : Specify the Location of Detailed Placer
* __-fast__ : Fast and quick placement for IO-pin placement. Please do NOT use this command in general purposes (It'll not spread all cells enough)

## Timing-Driven mode for Lef/Def/Verilog (Commercial Format)
    $ ./RePlAce -bmflag <mms/ispd/sb/ibm/etc> -lef tech.lef -lef macro.lef ...  -def <*.def> -verilog <*.v> -lib lib1.lib -lib lib2.lib ... -sdc <*.sdc> -timing -capPerMicron 0.23e-15 -resPerMicron 70.0 -output <outputLocation> -dpflag <NTU3/NTU4> -dploc <dpLocation> [Options]

__Timing-Driven__ mode must have same arguments as non-timing mode, but the __differences__ are:
* __-timing__ : Specify the Timing-Driven Placement Mode
* __-verilog__ : \*.v Location (__Required__ for OpenSTA)
* __-sdc__ : Specify the Synopsys Design Constraint (SDC) file. (Required for OpenSTA)
* __-lib__ : \*.lib Location (Multiple lib files supported. Required for OpenSTA)
* __-capPerMicron__ : Capacitance per Micron. Unit: Farad. (Used for Internal RC Extraction)
* __-resPerMicron__ : Resisance per Micron. Unit: Ohm. (Used for Internal RC Extraction)

## Legalization mode 
    $ ./RePlAce -bmflag etc -onlyDP -onlyLG -lef tech.lef -lef macro.lef ...  -def <*.def> -output <outputLocation> -dpflag <NTU3/NTU4> -dploc <dpLocation>

Note that verilog is not supported in this mode.

__Legalization-Only__ mode must have same arguments as original mode, but the __differences__ are:

* __-onlyDP__ : Only Detailed Placement Mode
* __-onlyLG__ : Call Detailed Placement in Legalization Mode

## Other Options
### Flow Control
* __-timing__ : Enable Timing-Driven Placement Mode 
* __-skipIP__ : Skip Initial Placement Mode 
* __-onlyGP__ : Only Global Placement Mode
* __-onlyDP__ : Only Detailed Placement Mode
* __-onlyLG__ : Call Detailed Placement in Legalization Mode

### Nesterov Control
* __-den__ : Target Density, Floating Number, Default = 1 [0.00,1.00]
* __-bin__ : #bins (in power of 2) for x, y, z Directions, Unsigned Integer[3], Default = 32 32 32
* __-overflow__ : Overflow Termination Condition, Floating Number, Default = 0.1 [0.00, 1.00]
* __-pcofmin__ : µ_k Lower Bound, Float Number, Default = 0.95
* __-pcofmax__ : µ_k Upper Bound, Float Number, Default = 1.05
* __-rancti__ : Max # Global Router Calls and Cell Inflation. No Restore of Cell Size, Keep Increasing, Unsigned Integer, Default = 10
* __-maxinfl__ : Max Cell Inflation for Each Cell Inflation Per Global Router Call. Floating Number, Default = 2.5
* __-inflcoef__ : γ_super, Floating Number, Default = 2.33
* __-filleriter__ : # Filler Only Placement Iterations, Floating Number, Default = 20
* __-stepScale__ : ∆HPWL_REF, Floating Number, Default=346000

### Plot
* __-plot__ : Plot Layout Every 10 Iterations (Cell, Bin, Arrow Plots)

### Router
* __-routability__ : Enable Routability flow 

### Other
* __-unitY__ : Custom scaledown param for LEF/DEF/Verilog (especially for ASAP N7 library; Recommended: 864)
