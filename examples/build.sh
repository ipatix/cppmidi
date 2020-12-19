#!/bin/sh

set -eu

for file in *.cpp; do
    g++ -Wall -Wextra -O2 ../cppmidi.cpp -I .. $file -o ${file%.cpp}
done
