#!/bin/bash

SCAD="openscad.exe"
OUTDIR="VintageKnobD"

mkdir -pv $OUTDIR

echo "Generating knobs and caps..."
FILE1="VintageKnobD.scad"
FILE2="VintageKnobDcap.scad"
for SHAFT in D Knurled; do
  echo " Shaft: $SHAFT"
  # Dimensional data: NAME MAJOR_DIAMETER MINOR_DIAMETER HEIGHT
  for DATA in "S 18 11 16" "M 22 13 16" "L 26 16 20" "XL 34 20 24"; do
    SIZE=($DATA)
    LABEL="${SIZE[0]}"
    MAJOR_DIAMETER="${SIZE[1]}"
    MINOR_DIAMETER="${SIZE[2]}"
    HEIGHT="${SIZE[3]}"
    echo "  $LABEL: $MAJOR_DIAMETER x $MINOR_DIAMETER x $HEIGHT"
    $SCAD -o "$OUTDIR/VintageKnobD_${LABEL}_${SHAFT}.stl" \
      -D "MajorDiameter=$MAJOR_DIAMETER" \
      -D "MinorDiameter=$MINOR_DIAMETER" \
      -D "Height=$HEIGHT" \
      -D "ShaftType=\"$SHAFT\"" \
      "$FILE1"
    $SCAD -o "$OUTDIR/VintageKnobDcap_${LABEL}.stl" \
      -D "MinorDiameter=$MINOR_DIAMETER" \
      "$FILE2"
  done
done
