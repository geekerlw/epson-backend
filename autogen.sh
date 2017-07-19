#!/bin/sh

# 
# Copyright (c) 2016-2017
# 
# yinka-utils autogen.sh
#
# Run this to generate all the initial makefiles, etc.

mkdir m4
autoreconf -vfi $@
