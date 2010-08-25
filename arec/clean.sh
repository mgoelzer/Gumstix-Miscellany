#!/bin/sh

VOLATILE_DIR=/media/ram
NONVOLATILE_DIR=/home/root/arec

rm $VOLATILE_DIR/*.wav $VOLATILE_DIR/data.in $VOLATILE_DIR/data.fft 2>/dev/null
rm $NONVOLATILE_DIR/*.png 2>/dev/null
