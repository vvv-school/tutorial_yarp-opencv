# Copyright: (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
# Authors: Vadim Tikhanoff
# CopyPolicy: Released under the terms of the GNU GPL v2.0.
#
# yarp-opencv.thrift

/**
* yarp-opencv_IDL
*
* IDL Interface to \ref Yarp OpenCV Module.
*/

service yarpOpencv_IDL
{
    /**
     * Set the lower bound of the colour threshold
     * @param r: Red value of the lower bound
     * @param g: Green value of the lower bound
     * @param b: Blue value of the lower bound
     * @return true/false on success/failure
     */
    bool setLowerBound(1:i32 r, 2:i32 g, 3:i32 b);

    /**
     * Set the upper bound of the colour threshold
     * @param r: Red value of the lower bound
     * @param g: Green value of the lower bound
     * @param b: Blue value of the lower bound
     * @return true/false on success/failure
     */
    bool setUpperBound(1:i32 r, 2:i32 g, 3:i32 b);
    
    /**
     * Gets the lower bound of the colour threshold
     * @return list of RBG values
     */
    list<i32> getLowerBound();
    
    /**
     * Gets the higher bound of the colour threshold
     * @return list of RBG values
     */
    list<i32> getUpperBound();
    
    /**
     * Set the dilate number of iteration
     * @param iter: number of dilate iteration
     * @return true/false on success/failure
     */
    bool setDilateIter(1:i32 iter);
    
    /**
     * Set the erode number of iteration
     * @param iter: number of erode iteration
     * @return true/false on success/failure
     */
    bool setErodeIter(1:i32 iter);
    
    /**
     * Set the gausian blur size (note that the number needs to be odd for the kernel)
     * @param size: number of the gausian size
     * @return true/false on success/failure
     */
    bool setGausianSize(1:i32 size);
    
    /**
     * Gets the dilate iter number
     * @return i32 the iter
     */
    i32 getDilateIter();
    
    /**
     * Gets the dilate iter number
     * @return i32 the iter
     */
    i32 getErodeIter();
    
    /**
     * Gets the gausian blur size
     * @return i32 the gausian blur size
     */
    i32 getGausianSize();
    
    /**
     * Quit the module.
     * @return true/false on success/failure
     */
    bool quit();
}
