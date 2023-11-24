#!/bin/bash
mkdir build
cd build
cmake -S ../ -B ./
make

mv host* ../

cd ../
rm -r build