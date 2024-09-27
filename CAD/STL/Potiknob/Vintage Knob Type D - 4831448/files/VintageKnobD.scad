// Major diameter (bottom of flange)
MajorDiameter = 18;
// Minor diameter (main body)
MinorDiameter = 11;
// Total height
Height = 16;
// Taper (body top diameter/bottom diameter). 1.0=no taper 0.6=heavy taper
Taper = 0.95; // [0.6:0.01:1.0]
// Depth of white indicator line. Increase if needed to account for sanding.
IndicatorLineDepth = 0.5;

/* [Shaft] */

// Potentiometer/encoder shaft type
ShaftType = "D"; // ["D", "Knurled", "None"]
// Depth (length) of shaft.
ShaftDepth = 10; // [0.0:0.01:50.0]
// X/Y Offset to compensate for shrinking holes when printing. Increase if too tight. Decrease if too loose.
ShaftOffset = 0.3; // [-1.0:0.01:1.0]

// Shaft diameter (D-shaft) ->O<-
DShaftMajorDiameter = 6.0; // [3.0:0.01:15.0]
// Minor diameter (across cut) (D-shaft) ->D<-
DShaftMinorDiameter = 4.2; // [3.0:0.01:15.0]

// Major diameter (knurled shaft)
KnurledShaftMajorDiameter = 6.0; // [3.0:0.01:15.0]
// Number of serrations/knurls per recolution (knurled shaft)
KnurlCount = 18;




// Utility constants
e=0.001; // error term (e.g. for nudging coincident surfaces for CSG)
in=25.4; // in => mm



// Slant angle for a cylinder with h and diameters d1 and d2
function Slant(h, d1, d2) = atan(0.5*(d1-d2)/h);

// Diameter of a slanted cylinder at height pos (lerp)
function DiameterAt(h, d1, d2, pos) = d1 - (d1-d2)*(pos/h);


module Cone(h, d) {
  cylinder(h=h, d1=d, d2=0);
}

module Tube(h, d1, d2) {
  difference(){
    cylinder(h=h, d=max(d1,d2));
    translate([0,0,-e]) cylinder(h=h+2*e, d=min(d1,d2));
  };
}

// Cylinder with bevel along top edge
module BeveledCylinder(h, d1, d2, bevel, bevelBottom=false) {
  r2 = d2*0.5;
  intersection() {
    cylinder(h=h, d1=d1, d2=d2);
    translate([0,0,-bevel])
      Cone(h=h+r2, d=2*r2*(1+h/r2));
    if (bevelBottom) {
      translate([0,0,-r2+bevel])
        cylinder(h=h+r2, d1=0, d2=d1*(1+h/r2));}  
  };
}

// Cylinder with bevel along top edge
module BeveledCylinder2(h, d1, d2, bevelx, bevelz) {
  difference() {
    cylinder(h=h, d1=d1, d2=d2);
    translate([0,0,h+e])
      rotate_extrude()
        translate([d2/2-bevelx+e,0,0])
          polygon([[0,0],[bevelx,-bevelz]*2,[bevelx,0]*2]);
  };
}



// Obsolete: Use Serrations
module SquareSerrations(h, d1, d2, fromz, toz, offset=0, count, gapPct=50, rotationDeg, slant) {
  baseD = DiameterAt(h, d1, d2, fromz);
  side = baseD*PI / count * (100-gapPct)/100;
  for (i=[0:count-1])
      rotate([0,0,360/count*i])
        translate([baseD/2+offset, 0, fromz])
          rotate([0, -slant, 0])
            rotate([0,0,rotationDeg])
              translate([-side/2, -side/2, 0])
                cube([side,side,toz-fromz]);
}



module Flange(h1, h2, d1, d2, capAt=0) {
  cylinder(d=d1, h=h1);
  intersection() {
    translate([0,0,h1]) cylinder(h=h2, d1=d1, d2=d2);
    if (capAt > 0) cylinder(h=capAt, d=d1);
  }
}

module Dome(h, d, cut=true) {
  x = d / 2;
  r = (x*x+h*h)/(2*h);
  y = r - h;
  difference() {
    translate([0,0,-y]) sphere(r=abs(r)-e);
    if (cut) translate([0,0,-r]) cube([abs(r)*2,abs(r)*2,abs(r)*2], center=true);
  }
}

module LineMarker(fromz, toz, d, widthDeg=5, lengthPct=70) {
  circ = 2*PI*d;
  width = circ*widthDeg/360;
  length = d/2*lengthPct/100;
  translate([-length,-width/2,fromz])
    cube([length,width,toz-fromz]);
}





// Stronger thin-wall d-shaft (hopefully more flexible for a better fit)
// d = shaft diameter ->O<-
// dmin = minor diameter (across cut) ->D<-
module DShaft5(h, d=5.9, dmin=4.3) {
    //wallThickness = 1.2;
    wallThickness = 0.8;
    outerCylinderD = d + 2*wallThickness + 1.2;
    flatXpos = (d/2) - (d-dmin);
    
    difference() {
        cylinder(h=h, d=outerCylinderD);
        translate([0,0,-e]) {
            difference() {
                Tube(h=h, d1=d + 2*wallThickness, d2=d);
                //translate([flatXpos-wallThickness,-outerCylinderD/2,0]) cube([outerCylinderD,outerCylinderD,h]);
                translate([-d-wallThickness, -wallThickness/2, 0]) cube([d+wallThickness,wallThickness,h]);
        }};
        translate([flatXpos, -d*0.45, 0])  // TODO: Adjust based on dMin
            cube([wallThickness, d*0.9, h]); // TODO: Adjust based on dMin
    }
}

// Knurled shaft.
// 6mm major diameter, 18 knurls, fits most pots.
module KnurledShaft(h, clearance=0, flare=0, d=6, count=18) {
  spacing = (d-2*clearance) * PI/count * 0.7;
  difference() {
    BeveledCylinder(h-clearance, d+2*clearance, d+2*clearance, 0.5);
    SquareSerrations(h=h, d1=d+2*clearance, d2=d+2*clearance, fromz=0, toz=h,
      count=18, gapPct=30, rotationDeg=45, slant=0);
  }
  if (flare>0) Cone(h=d/2+flare*2, d=d+flare*2);
}

module Shaft() {
    translate([0,0,-e]) {
        if (ShaftType == "D") {
            DShaft5(h=ShaftDepth,
                d=DShaftMajorDiameter+ShaftOffset,
                dmin=DShaftMinorDiameter+ShaftOffset);
        }
        else if (ShaftType == "Knurled") {
            KnurledShaft(h=ShaftDepth,
                clearance=ShaftOffset,
                d=KnurledShaftMajorDiameter,
                count=KnurlCount);
        }
    };
}


$fn = $preview ? 45 : 60;

module VintageKnobD(dmaj, dmin, h)
{
  d2 = dmin * Taper;
  pointerWidth = dmaj * 0.12; // at tip
  difference(){
    union() {
      // Main body
      BeveledCylinder2(h=h, d1=dmin, d2=d2, bevelx=dmin*0.03, bevelz=dmin*0.03);
      // Flange  
      Flange(h1=dmaj*0.05, h2=dmaj*0.25, d1=dmaj, d2=0);
      // Pointer
      difference(){
        yOffset = MajorDiameter*2;
        // outer body, with tick marks
        union() {
          // smaller body
          BeveledCylinder2(h=h-IndicatorLineDepth, d1=dmaj-IndicatorLineDepth, d2=dmaj*Taper-IndicatorLineDepth, bevelx=dmaj*0.03, bevelz=dmaj*0.03);
          difference() {
            // larger body
            BeveledCylinder2(h=h, d1=dmaj, d2=dmaj*Taper, bevelx=dmaj*0.03, bevelz=dmaj*0.03);  
            // line mark cut out of larger body
            translate([-MajorDiameter, -pointerWidth/5, 0])
              cube([MajorDiameter, pointerWidth/2.5, Height+e]);
          }
        }
        translate([-MajorDiameter/2,yOffset+pointerWidth/2,0]) cylinder(h=h+e, r=yOffset);
        translate([-MajorDiameter/2,-yOffset-pointerWidth/2,0]) cylinder(h=h+e, r=yOffset);
        translate([dmaj/1.24,0,0]) cylinder(h=h+e, d=dmaj/1.2);        
      }
    };
    translate([0,0,h-1]) cylinder(h=2,d=d2*0.8);
    translate([0,0,-e]) Shaft();
  }
}

VintageKnobD(dmaj=MajorDiameter, dmin=MinorDiameter, h=Height);