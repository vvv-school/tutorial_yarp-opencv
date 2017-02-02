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
#include <yarp/os/Semaphore.h>
#include <yarp/sig/Image.h>

#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>

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
    
    yarp::os::Mutex mutex;
    
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

        cv::Mat in_cv = cv::cvarrToMat((IplImage *)img.getIplImage());

        cv::Mat redBallOnly;
        
        //
        //let's fill things in
        //
        
        if (outTargets.size() > 0)
            targetPort.write();

        IplImage colour = in_cv;
        outImage.resize(colour.width, colour.height);
        cvCopy( &colour, (IplImage *) outImage.getIplImage());
        outPort.write();

        //IplImage edges = redBallOnly;
        //outEdges.resize(edges.width, edges.height);
        //cvCopy( &edges, (IplImage *) outEdges.getIplImage());
        //edgesPort.write();
    }

    /********************************************************/
    bool setLowerBound(const int32_t r, const int32_t g, const int32_t b)
    {
        mutex.lock();
        lowBound.clear();
        lowBound.push_back(r);
        lowBound.push_back(g);
        lowBound.push_back(b);
        mutex.unlock();
        return true;
    }
    /********************************************************/
    bool setUpperBound(const int32_t r, const int32_t g, const int32_t b)
    {
        mutex.lock();
        highBound.clear();
        highBound.push_back(r);
        highBound.push_back(g);
        highBound.push_back(b);
        mutex.unlock();
        return true;
    }
    
    /********************************************************/
    bool setDilateIter(const int32_t iter)
    {
        mutex.lock();
        dilate_niter = iter;
        mutex.unlock();
        return true;
    }
    
    /********************************************************/
    bool setErodeIter(const int32_t iter)
    {
        mutex.lock();
        erode_niter = iter;
        mutex.unlock();
        return true;
    }
    
    /********************************************************/
    bool setGausianSize(const int32_t size)
    {
        mutex.lock();
        gausian_size = size;
        mutex.unlock();
        return true;
    }
    
    /********************************************************/
    std::vector<int32_t> getLowerBound()
    {
        std::vector<int32_t> v;
        mutex.lock();
        v = lowBound;
        mutex.unlock();
        
        return v;
    }
    
    /********************************************************/
    std::vector<int32_t> getUpperBound()
    {
        std::vector<int32_t> v;
        mutex.lock();
        v = highBound;
        mutex.unlock();
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

        rpcPort.open(("/"+getName("/rpc")).c_str());

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
        return 1;
    }

    Module module;
    yarp::os::ResourceFinder rf;

    rf.setVerbose();
    rf.configure(argc,argv);

    return module.runModule(rf);
}
