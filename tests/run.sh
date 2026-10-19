#!/bin/sh
# Builds and runs the pure C++ core tests (no JUCE needed): sh tests/run.sh
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/stretch_test test_core.cpp
/tmp/stretch_test
