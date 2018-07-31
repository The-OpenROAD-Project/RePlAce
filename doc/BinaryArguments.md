# Usage
## Bookshelf    
    $ ./RePlACE -bmflag <mms/ispd/sb/ibm/etc> -aux <*.aux> -output <outputLocation> -dpflag <NTU3/NTU4> -dploc <dpLocation> [Options]
    
* __-bmflag__ : Specify which Benchmark is used
* __-aux__ : \*.aux Location

## Lef/Def/Verilog
    $ ./RePlACE -bmflag <mms/ispd/sb/ibm/etc> -lef <*.lef> -def <*.def> [-v <*.v>] -output <outputLocation> -dpflag <NTU3/NTU4> -dploc <dpLocation> [Options]

* __-bmflag__ : Specify which Benchmark is used
* __-lef__ : \*.lef Location
* __-def__ : \*.def Location
* __-v__ : \*.v Location (Optional)

## Options
### Placement
* __-onlyGP__ : Only Global Placement Mode
* __-den__ : Target Density, Floating Number, Default = 1 [0.00,1.00]
* __-bin__ : #bins (in power of 2) for x, y, z Directions, Unsigned Integer[3], Default = 32 32 32
* __-overflow__ : Overflow Termination Condition, Floating Number, Default = 0.1 [0.00, 1.00]
* __-pcofmin__ : µ_k Lower Bound, Float Number, Default = 0.95
* __-pcofmax__ : µ_k Upper Bound, Float Number, Default = 1.05
* __-rancti__     : Max # Global Router Calls and Cell Inflation. No Restore of Cell Size, Keep Increasing, Unsigned Integer, Default = 10
* __-maxinfl__    : Max Cell Inflation for Each Cell Inflation Per Global Router Call. Floating Number, Default = 2.5
* __-inflcoef__   : γ_super, Floating Number, Default = 2.33
* __-filleriter__ : # Filler Only Placement Iterations, Floating Number, Default = 20
* __-stepScale__  : ∆HPWL_REF, Floating Number, Default=346000

### Plot
* __-aplot__      : Plot Layout Every 10 Iterations

### Detail Placer
* __-dpflag__     : Specify which Detailed Placer is Used
* __-dploc__      : Specify the Location of Detailed Placer
* __-onlyLG__     : Call Detailed Placement in Legalization Mode

### Router
* __-R__          : Enable Routability flow (currently only supports DAC-2012 and ICCAD-2012 benchmark suites)

### Other
* __-unitY__      : Custom scaledown param for LEF/DEF/Verilog (especially for ASAP N7 library)
