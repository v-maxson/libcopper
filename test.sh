#!/bin/sh
set -e

cmake --preset debug
cmake --build --preset debug
ctest --preset debug
