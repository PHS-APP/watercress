#!/bin/sh
set -eu
gcc -O0 -std=c99 -pedantic-errors -o watercress watercress.c
exit
