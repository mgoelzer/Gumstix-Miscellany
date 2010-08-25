#!/bin/bash

VOLATILE_DIR=/media/ram
NONVOLATILE_DIR=/home/root/arec

cd $VOLATILE_DIR

K=$((1000))
echo -n "$K rounds"

echo "0 1 00 01 10 11" >> $NONVOLATILE_DIR/stats.txt
SAMPLE_RATE=44100   #176000

for N in $(seq 1 $K); do
	arecord -d 30 -f S16_LE -c 1 -r $SAMPLE_RATE -t wav arec-$N.wav 2>/dev/null
	dd if=arec-$N.wav of=tmp.wav bs=2 skip=$((2*$SAMPLE_RATE)) 2>/dev/null && rm arec-$N.wav
	$NONVOLATILE_DIR/fft tmp.wav $SAMPLE_RATE $NONVOLATILE_DIR/rand.bin NOFFT 2>/dev/null >> $NONVOLATILE_DIR/stats.txt
	#$NONVOLATILE_DIR/genplot.pl $VOLATILE_DIR/data.in $NONVOLATILE_DIR/plot-raw-$N.png $VOLATILE_DIR/data.fft $NONVOLATILE_DIR/plot-fft-$N.png
	rm tmp.wav #$VOLATILE_DIR/data.fft $VOLATILE_DIR/data.in

#	arecord --separate-channels -d 30 -f S16_LE -c 2 -r $SAMPLE_RATE -t wav arec-short-$N.wav 2>/dev/null
#	dd if=arec-short-$N.wav.0 of=tmp.wav bs=2 skip=$((7*$SAMPLE_RATE)) 2>/dev/null && rm arec-short-$N.wav.0
#	$NONVOLATILE_DIR/fft tmp.wav $SAMPLE_RATE $NONVOLATILE_DIR/rand.bin FFT 2>/dev/null >> $NONVOLATILE_DIR/stats.txt
#	rm tmp.wav $VOLATILE_DIR/data.in $VOLATILE_DIR/data.fft > /dev/null
#
#	dd if=arec-short-$N.wav.1 of=tmp.wav bs=2 skip=$((7*$SAMPLE_RATE)) 2>/dev/null && rm arec-short-$N.wav.1
#	$NONVOLATILE_DIR/fft tmp.wav $SAMPLE_RATE $NONVOLATILE_DIR/rand.bin FFT 2>/dev/null >> $NONVOLATILE_DIR/stats.txt
#	$NONVOLATILE_DIR/genplot.pl $VOLATILE_DIR/data.in $NONVOLATILE_DIR/plot-raw-$N.png $VOLATILE_DIR/data.fft $NONVOLATILE_DIR/plot-fft-$N.png
#	rm tmp.wav $VOLATILE_DIR/data.in $VOLATILE_DIR/data.fft >/dev/null
	echo -n "."
done

echo "done."
