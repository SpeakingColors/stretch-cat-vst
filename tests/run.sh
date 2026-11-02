#!/bin/sh
# Builds and runs the pure C++ core tests (no JUCE needed): sh tests/run.sh
# Also runs them with address + undefined sanitizers, and with the thread sanitizer for the render worker (SANITIZE=0 to skip).
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -O2 -Wall -Wextra -pthread -o /tmp/stretch_test test_core.cpp
/tmp/stretch_test
if [ "${SANITIZE:-1}" = "1" ]; then
  g++ -std=c++17 -O1 -g -pthread -fsanitize=address,undefined -fno-sanitize-recover=undefined -o /tmp/stretch_test_asan test_core.cpp
  /tmp/stretch_test_asan > /tmp/stretch_asan.log 2>&1 && echo "asan+ubsan: clean" || { tail -30 /tmp/stretch_asan.log; exit 1; }
  g++ -std=c++17 -O1 -g -pthread -fsanitize=thread -o /tmp/stretch_test_tsan test_core.cpp
  /tmp/stretch_test_tsan > /tmp/stretch_tsan.log 2>&1 && echo "tsan: clean" || { tail -40 /tmp/stretch_tsan.log; exit 1; }
fi
