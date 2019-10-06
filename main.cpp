/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Vadim Tikhanoff
 * email:  vadim.tikhanoff@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

#include <yarp/os/BufferedPort.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Network.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/sig/Image.h>
#include <yarp/cv/Cv.h>

#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

#include <cstdlib>
#include <mutex>
#include <vector>
#include <iostream>

#include "yarpOpencv_IDL.h"

const int HIGH_THRESHOLD = 80;
const int HOUGH_MIN_VOTES = 15;
const int HOUGH_MIN_RADIUS = 10;
const int HOUGH_MAX_RADIUS = 30;

/********************************************************/
class Processing : public yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >
{
    std::string moduleName;

    yarp::os::RpcServer handlerPort;

    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >    outPort;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelMono> >   edgesPort;
    yarp::os::BufferedPort<yarp::os::Bottle>  targetPort;

    std::vector<int32_t> lowBound;
    std::vector<int32_t> highBound;
    
    std::mutex mtx;
    
    int dilate_niter;
    int erode_niter;
    int gausian_size;

public:
    /********************************************************/

    Processing( const std::string &moduleName )
    {
        this->moduleName = moduleName;
    }

    /********************************************************/
    ~Processing()
    {

    };

    /********************************************************/
    bool open(){

        this->useCallback();

        BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >::open( "/" + moduleName + "/image:i" );
        outPort.open("/" + moduleName + "/image:o");
        edgesPort.open("/" + moduleName + "/mask:o");
        targetPort.open("/"+ moduleName + "/target:o");

        //magical values
        lowBound.push_back(200);
        lowBound.push_back(0);
        lowBound.push_back(0);
        
        highBound.push_back(255);
        highBound.push_back(41);
        highBound.push_back(20);
        
        dilate_niter = 4;
        erode_niter = 0;
        gausian_size = 9;
        
        return true;
    }

    /********************************************************/
    void close()
    {
        outPort.close();
        edgesPort.close();
        targetPort.close();
        BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >::close();
    }

    /********************************************************/
    void interrupt()
    {
        BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >::interrupt();
    }

    /********************************************************/
    void onRead( yarp::sig::ImageOf<yarp::sig::PixelRgb> &img )
    {
        yarp::sig::ImageOf<yarp::sig::PixelRgb> &outImage  = outPort.prepare();
        yarp::sig::ImageOf<yarp::sig::PixelMono> &outEdges = edgesPort.prepare();
        yarp::os::Bottle &outTargets = targetPort.prepare();

        outImage.resize(img.width(), img.height());
        outEdges.resize(img.width(), img.height());

        outEdges.zero();

        cv::Mat in_cv = yarp::cv::toCvMat(outImage);
        outImage = img;

        cv::Mat redBallOnly = yarp::cv::toCvMat(outEdges);
        
        mtx.lock();
        //void inRange(InputArray src, InputArray lowerb, InputArray upperb, OutputArray dst)
        cv::inRange(in_cv, cv::Scalar(lowBound[0], lowBound[1], lowBound[2]), cv::Scalar(highBound[0], highBound[1], highBound[2]), redBallOnly);

        //void GaussianBlur(InputArray src, OutputArray dst, Size ksize, double sigmaX=0, double sigmaY=0, int borderType=BORDER_DEFAULT )
        cv::GaussianBlur(redBallOnly, redBallOnly, cv::Size(gausian_size, gausian_size), 2, 2);

        //void dilate(InputArray src, OutputArray dst, InputArray kernel, Point anchor=Point(-1,-1), int iterations=1, int borderType=BORDER_CONSTANT, const Scalar& borderValue=morphologyDefaultBorderValue()
        cv::dilate(redBallOnly, redBallOnly, cv::Mat(), cv::Point(-1,-1), dilate_niter, cv::BORDER_CONSTANT, cv::morphologyDefaultBorderValue());
        
         //void erode(InputArray src, OutputArray dst, InputArray kernel, Point anchor=Point(-1,-1), int iterations=1, int borderType=BORDER_CONSTANT, const Scalar& borderValue=morphologyDefaultBorderValue() )
        cv::erode(redBallOnly, redBallOnly, cv::Mat(), cv::Point(-1,-1), erode_niter, cv::BORDER_CONSTANT, cv::morphologyDefaultBorderValue());
        mtx.unlock();
        
        std::vector<cv::Vec3f> circles;
        //void HoughCircles(InputArray image, OutputArray circles, int method, double dp, double minDist, double param1=100, double param2=100, int minRadius=0, int maxRadius=0 )
        cv::HoughCircles(redBallOnly, circles, CV_HOUGH_GRADIENT, 1, redBallOnly.rows / 8, HIGH_THRESHOLD, HOUGH_MIN_VOTES, HOUGH_MIN_RADIUS, HOUGH_MAX_RADIUS);

        yDebug("Found %lu circles", circles.size());

        outTargets.clear();

        // Draw the circles detected
        for (size_t i = 0; i < circles.size(); i++)
        {
            cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
            int radius = cvRound(circles[i][2]);
            // circle center
            circle(in_cv, center, 3, cv::Scalar(0, 255, 0), -1, 8);
            // circle outline
            circle(in_cv, center, radius, cv::Scalar(0, 0, 255), 3, 8);

            yarp::os::Bottle &t=outTargets.addList();
            t.addDouble(center.x);
            t.addDouble(center.y);
        }
        
        if (outTargets.size() > 0)
            targetPort.write();

        outPort.write();
        edgesPort.write();
    }

    /********************************************************/
    bool setLowerBound(const int32_t r, const int32_t g, const int32_t b)
    {
        lock_guard<mutex> lck(mtx);
        lowBound.clear();
        lowBound.push_back(r);
        lowBound.push_back(g);
        lowBound.push_back(b);
        return true;
    }
    /********************************************************/
    bool setUpperBound(const int32_t r, const int32_t g, const int32_t b)
    {
        lock_guard<mutex> lck(mtx);
        highBound.clear();
        highBound.push_back(r);
        highBound.push_back(g);
        highBound.push_back(b);
        return true;
    }
    
    /********************************************************/
    bool setDilateIter(const int32_t iter)
    {
        lock_guard<mutex> lck(mtx);
        dilate_niter = iter;
        return true;
    }
    
    /********************************************************/
    bool setErodeIter(const int32_t iter)
    {
        lock_guard<mutex> lck(mtx);
        erode_niter = iter;
        return true;
    }
    
    /********************************************************/
    bool setGausianSize(const int32_t size)
    {
        lock_guard<mutex> lck(mtx);
        gausian_size = size;
        return true;
    }
    
    /********************************************************/
    std::vector<int32_t> getLowerBound()
    {
        lock_guard<mutex> lck(mtx);
        std::vector<int32_t> v;
        v = lowBound;     
        return v;
    }
    
    /********************************************************/
    std::vector<int32_t> getUpperBound()
    {
        lock_guard<mutex> lck(mtx);
        std::vector<int32_t> v;
        v = highBound;
        return v;
    }
    
    /********************************************************/
    int32_t getDilateIter()
    {
        return dilate_niter;
    }
    
    /********************************************************/
    int32_t getErodeIter()
    {
        return erode_niter;
    }
    
    /********************************************************/
    int32_t getGausianSize()
    {
        return gausian_size;
    }
};


/********************************************************/
class Module : public yarp::os::RFModule, public yarpOpencv_IDL
{
    yarp::os::ResourceFinder    *rf;
    yarp::os::RpcServer         rpcPort;

    Processing                  *processing;
    friend class                processing;

    bool                        closing;

    /********************************************************/
    bool attach(yarp::os::RpcServer &source)
    {
        return this->yarp().attachAsServer(source);
    }

public:

    /********************************************************/
    bool setLowerBound(const int32_t r, const int32_t g, const int32_t b)
    {
        processing->setLowerBound(r, g, b);
        return true;
    }
    /********************************************************/
    bool setUpperBound(const int32_t r, const int32_t g, const int32_t b)
    {
        processing->setUpperBound(r, g, b);
        return true;
    }
    
    /********************************************************/
    bool setDilateIter(const int32_t iter)
    {
        processing->setDilateIter(iter);
        return true;
    }
    
    /********************************************************/
    bool setErodeIter(const int32_t iter)
    {
        processing->setErodeIter(iter);
        return true;
    }
    
    /********************************************************/
    bool setGausianSize(const int32_t size)
    {
        bool success = true;
        if (size & 0x01)
            processing->setGausianSize(size);
        else
            success = false;
        
        return success;
    }
    
    /********************************************************/
    std::vector<int32_t> getLowerBound()
    {
        return processing->getLowerBound();
    }
    
    /********************************************************/
    std::vector<int32_t> getUpperBound()
    {
        return processing->getUpperBound();
    }
    
    /********************************************************/
    int32_t getDilateIter()
    {
        return processing->getDilateIter();
    }
    
    /********************************************************/
    int32_t getErodeIter()
    {
        return processing->getErodeIter();
    }
    /********************************************************/
    int32_t getGausianSize()
    {
        return processing->getGausianSize();
    }
    
    /********************************************************/
    bool configure(yarp::os::ResourceFinder &rf)
    {
        this->rf=&rf;
        std::string moduleName = rf.check("name", yarp::os::Value("yarp-opencv"), "module name (string)").asString();
        setName(moduleName.c_str());

        rpcPort.open("/"+getName("/rpc"));

        closing = false;

        processing = new Processing( moduleName );

        /* now start the thread to do the work */
        processing->open();

        attach(rpcPort);

        return true;
    }

    /**********************************************************/
    bool close()
    {
        processing->interrupt();
        processing->close();
        delete processing;
        return true;
    }

    /**********************************************************/
    bool quit(){
        closing = true;
        return true;
    }

    /********************************************************/
    double getPeriod()
    {
        return 0.1;
    }

    /********************************************************/
    bool updateModule()
    {
        return !closing;
    }
};

/********************************************************/
int main(int argc, char *argv[])
{
    yarp::os::Network::init();

    yarp::os::Network yarp;
    if (!yarp.checkNetwork())
    {
        yError("YARP server not available!");
        return EXIT_FAILURE;
    }

    Module module;
    yarp::os::ResourceFinder rf;

    rf.setVerbose();
    rf.configure(argc,argv);

    return module.runModule(rf);
}
