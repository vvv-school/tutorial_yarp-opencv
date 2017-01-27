#!/bin/bash

# Copyright: (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Vadim Tikhanoff <vadim.tikhanoff@iit.it>
# CopyPolicy: Released under the terms of the GNU GPL v3.0.

# color codes
red='\033[1;31m'
green='\033[1;32m'
nc='\033[0m'

code_dir=$(pwd)/../
test_dir=$(pwd)

if [ -d build ]; then
    rm -Rf build
fi
mkdir build && cd build
build_dir=$(pwd)

git clone --depth 1 -b master https://github.com/vvv-school/vvv-school.github.io.git helpers
if [ $? -eq 0 ]; then
    if [ -f ${test_dir}/test-type ]; then
        test_type=$(head -1 ${test_dir}/test-type)
        ./helpers/scripts/smoke-test-${test_type}.sh $build_dir $code_dir $test_dir
        ret=$?
    else
        echo -e "${red}test-type is missing!${nc}"
        ret=4
    fi
else
    echo -e "${red}GitHub seems unreachable${nc}"
    ret=4
fi

cd ../
exit $ret
