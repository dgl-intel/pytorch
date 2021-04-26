#!/bin/bash

export USE_CUDNN=0
export REL_WITH_DEB_INFO=1
#export DEBUG=1
export CC=clang 
export CXX=clang++
export MAX_JOBS=16

git submodule sync
git submodule update --init --recursive


export CMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"}
python setup.py install
