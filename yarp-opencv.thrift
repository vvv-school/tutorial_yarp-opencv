# Copyright: (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Vadim Tikhanoff
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
#
# yarp-opencv.thrift

/**
* yarp-opencv_IDL
*
* IDL Interface to \ref Adapt Frames Module.
*/
service yarpOpencv_IDL
{
    /**
     * Quit the module.
     * @return true/false on success/failure
     */
    bool quit();
}
