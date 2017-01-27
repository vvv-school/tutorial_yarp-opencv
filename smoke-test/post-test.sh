#!/bin/bash

# Copyright: (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Vadim Tikhanoff <vadim.tikhanoff@iit.it>
# CopyPolicy: Released under the terms of the GNU GPL v3.0.

# Put here those instructions we need to execute after having run the test

echo "stop" | yarp rpc /yarpdataplayer/rpc:i
echo "quit" | yarp rpc /yarpdataplayer/rpc:i
