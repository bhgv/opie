#!/bin/sh

export QTDIR=$PWD/../qte-opie

export OPIEDIR=$PWD

export LD_LIBRARY_PATH=$OPIEDIR/lib:$QTDIR/lib:$LD_LIBRARY_PATH

make clean
make menuconfig
make

