#!/bin/sh

export QTDIR=$PWD/../qte-opie

export OPIEDIR=$PWD

export LD_LIBRARY_PATH=$OPIEDIR/lib:$QTDIR/lib:$LD_LIBRARY_PATH

export PATH=$QTDIR/bin:$OPIEDIR/bin:$PATH

export QWS_DISPLAY=QVFb:0

#qvfb&

qpe -qws

