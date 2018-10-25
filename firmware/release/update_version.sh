#!/bin/bash

V=`git describe`
PLACEHOLDER="GIT_VERSION_PLACEHOLDER-------"
VPATH=../inc/version.h.in
VOUT=../inc/version.h

sed -e "s/$PLACEHOLDER/$V/g" < $VPATH | tee $VOUT | grep GIT_VERSION
