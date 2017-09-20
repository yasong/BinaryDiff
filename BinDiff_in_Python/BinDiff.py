#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Author: Xiaokang Yin
# @Date:   2017-09-19 22:44:49
# @Last Modified by:   Xiaokang Yin
# @Last Modified time: 2017-09-20 08:16:07


#This program implemnts finding the differences between binary files.
#The first step is try to recode the c bindiff program with python
#then maybe change something to make it work better.
#

import sys
import math



#############
#
def usage():
    print "Binary Difference Tool"
    print
    print "Usage: BinDiff.py [-hcdstxC] [-b B] [-n N] [-f F] [-g G] [-y Y] file1 file2"
    print "    -h: show usage information and exit."
    print "    -c: concise output positions in decimal instead of hex."
    print "    -d: output byte positions in decimal instead of hex."
    print "    -s: stop after first block of differing bytes."
    print "    -t: print total number of bytes in differing blocks."
    print "    -x: output lenghts in hex instead of decimal."
    print "    -C: use capitals for hex values."
    print "    -b B: compare per block of B bytes."
    print "    -n N: treat N identical bytes or blocks in between two differing bytes/blocks."
    print "          as different, i.e. treat the whole as the same chunk."
    print "    -f F: dump each differing block to a file with as name byte position prefixed"
    print "          with F."
    print "    -g G: dump each original block to a file with as name byte position prefixed"
    print "          with G."
    print "    -y Y: compare file1 with a virtual file of same length, filled with bytes Y."
    print "          Y must be an integer between 0 and 255 (hex allowed if prefixed with 0x)."