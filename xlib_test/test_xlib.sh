#!/bin/sh

src/Debug/xasm ../xie_computer/xlib_test/test_$1.xasm 1.bin   ../xie_computer/xlib
read -p "Press enter to continue============================================"
src/Debug/xsim  1.bin