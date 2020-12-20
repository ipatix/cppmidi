#!/bin/sh

set -eu

for file in *.cpp; do
    g++ -std=c++17 -Wall -Wextra -g -Og ../cppmidi.cpp -I .. $file -o ${file%.cpp}
done
