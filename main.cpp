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

#include "yarpOpencv_IDL.h"

const int CANNY_HIGH_THRESHOLD = 80;
const int HOUGH_MIN_VOTES = 15;
const int HOUGH_MIN_RADIUS = 10;
const int HOUGH_MAX_RADIUS = 60;

/********************************************************/
class Processing : public yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >
{
    std::string moduleName;

    yarp::os::RpcServer handlerPort;

    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> >    outPort;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelMono> >   edgesPort;
    yarp::os::BufferedPort<yarp::os::Bottle>  targetPort;

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
        cv::inRange(in_cv, cv::Scalar(200, 0, 0), cv::Scalar(255, 41, 20), redBallOnly);

        cv::GaussianBlur(redBallOnly, redBallOnly, cv::Size(9, 9), 2, 2);

        int dilate_niter = 4;
        cv::dilate(redBallOnly, redBallOnly, cv::Mat(), cv::Point(-1,-1), 4, cv::BORDER_CONSTANT, cv::morphologyDefaultBorderValue());

        std::vector<cv::Vec3f> circles;

        cv::HoughCircles(redBallOnly, circles, CV_HOUGH_GRADIENT, 1, redBallOnly.rows / 8, CANNY_HIGH_THRESHOLD, HOUGH_MIN_VOTES, HOUGH_MIN_RADIUS, HOUGH_MAX_RADIUS);

        yDebug("Found %lu circles", circles.size());

        outTargets.clear();

        // Draw the circles detected
        for (size_t i = 0; i < circles.size(); i++)
        {
            cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
            int radius = cvRound(circles[i][2]);
            // circle center
            circle(in_cv, center, 3, cv::Scalar(0, 255, 0), -1, 8, 0);
            // circle outline
            circle(in_cv, center, radius, cv::Scalar(0, 0, 255), 3, 8, 0);

            yarp::os::Bottle &t=outTargets.addList();
            t.addDouble(center.x);
            t.addDouble(center.y);
        }

        if (outTargets.size() > 0)
            targetPort.write();

        IplImage colour = in_cv;
        outImage.resize(colour.width, colour.height);
        cvCopy( &colour, (IplImage *) outImage.getIplImage());
        outPort.write();

        IplImage edges = redBallOnly;
        outEdges.resize(edges.width, edges.height);
        cvCopy( &edges, (IplImage *) outEdges.getIplImage());
        edgesPort.write();
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
