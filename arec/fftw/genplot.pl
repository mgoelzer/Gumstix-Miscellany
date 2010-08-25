#!/usr/bin/perl

# Generate postscript and png plot with GNUplot
# (C) 2005 www.captain.at

$IN_RAW_NAME = shift;
$OUT_RAW_NAME = shift;
$IN_FFT_NAME = shift;
$OUT_FFT_NAME = shift;

open (GNUPLOT, "|gnuplot");
print GNUPLOT <<EOPLOT;
set term png small xFFFFFF
set output "$OUT_RAW_NAME"
set nokey
set data style points
set xlabel "Time" 
set autoscale
set xtic auto
set ytic auto
set title "Raw Audio Input"
set grid xtics ytics
plot "$IN_RAW_NAME" using 1:2 w points 1
EOPLOT
close(GNUPLOT);

open (GNUPLOT, "|gnuplot");
print GNUPLOT <<EOPLOT;
set term png small xFFFFFF
set output "$OUT_FFT_NAME"
set nokey
set data style line
set xlabel "Frequency [Hz]" 
set title "FFT of Audio Input"
set size 1 , 1
set xtic auto
set ytic auto
set yrange [0:100000]
set xrange [0:40000]
set grid xtics ytics
plot "$IN_FFT_NAME" using 1:2 w lines 1
EOPLOT
close(GNUPLOT);
