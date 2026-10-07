#!/usr/bin/env bash

cmake --preset linux-debug -DBUILD_TESTING=ON
cmake --build --preset linux-debug --target obliberry_tests --parallel 2
ctest --test-dir build/linux-debug --output-on-failure --no-tests=error
