#!/bin/bash

if ! test -d build; then
    mkdir build;
    cmake -B build
fi

cd build
make