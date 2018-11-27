# How to Report Memory Bugs For RePlAce

1) Modify Makefile that contains '-g -ggdb'
Change below Makefile below: 

    [RePlAce/Makefile](https://github.com/abk-openroad/RePlAce/blob/15e9f8aeb2b786a0b63419ab876ee222ac252f2c/src/Makefile#L4)

       OPTFLAG= -g -ggdb -m64 -O3 -fPIC -DNDEBUG -ffast-math -Dcimg_display=1 

2) Type below command

       $ make clean; $make

3) Turn [isValgrind = True] in below links:

    [RePlAce/src/execute_lefdef.py](https://github.com/abk-openroad/RePlAce/blob/15e9f8aeb2b786a0b63419ab876ee222ac252f2c/src/execute_lefdef.py#L28)

    If you are not using this script, then

       $ valgrind --log-fd=1 ./RePlAce (all options as in your picture) | tee design_valgrind.log

4) Report the *RePlAce/logdir/?_valgrind.log* to issues 
