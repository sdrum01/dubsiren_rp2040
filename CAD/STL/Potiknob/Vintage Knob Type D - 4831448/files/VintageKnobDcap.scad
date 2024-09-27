// Minor diameter (main body)
MinorDiameter = 11;
// Taper (body top diameter/bottom diameter). 1.0=no taper 0.6=heavy taper
Taper = 0.95; // [0.6:0.01:1.0]5


module VintageKnobACap(d, taper) {
    d2=d*taper;
    diameter = d2*0.85-0.5;
    cylinder(d=d*0.76-0.2, h=1, $fn=$fn*2);
}

$fn = $preview ? 15 : 40;
VintageKnobACap(d=MinorDiameter, taper=Taper);
