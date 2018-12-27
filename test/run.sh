
PLACER=NTU3
LEF="osu035_stdcells.lef"
DEF="map9v3.def"
OUT="out"
PLACE_LOC="/usr/local/bin/ntuplace3"

RePlAce -bmflag ispd -lef $LEF -def $DEF -output $OUT -dpflag $PLACER -dploc $PLACE_LOC -skipIP
