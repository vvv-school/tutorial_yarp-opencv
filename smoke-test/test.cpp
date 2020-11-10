/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Vadim Tikhanoff <vadim.tikhanoff@iit.it>
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
*/

#include <cstdlib>
#include <string>
#include <cmath>
#include <algorithm>

#include <yarp/robottestingframework/TestCase.h>
#include <robottestingframework/dll/Plugin.h>
#include <robottestingframework/TestAssert.h>


#include <yarp/os/Network.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/RpcClient.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/Time.h>

using namespace robottestingframework;

/**********************************************************************/
class TestTutorialYarpOpencv : public yarp::robottestingframework::TestCase
{
    yarp::os::RpcClient playerPort;
    yarp::os::BufferedPort<yarp::os::Bottle> targetPort;

public:
    /******************************************************************/
    TestTutorialYarpOpencv() :
        yarp::robottestingframework::TestCase("TestTutorialYarpOpencv")
    {
    }

    /******************************************************************/
    virtual ~TestTutorialYarpOpencv()
    {
    }

    /******************************************************************/
    virtual bool setup(yarp::os::Property& property)
    {
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("dataset"),"Dataset is unspecified!");
        std::string path = std::getenv("DATASETS_PATH");
        std::string dataset = property.find("dataset").asString();

        playerPort.open("/"+getName()+"/player:rpc");
        targetPort.open("/"+getName()+"/target:i");
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect(playerPort.getName(),"/yarpdataplayer/rpc:i"),
                                  "Unable to connect to yarpdataplayer!");
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/yarp-opencv/target:o",targetPort.getName()),
                                  "Unable to connect to target!");

        yarp::os::Bottle cmd,rep;
        cmd.addString("load");
        cmd.addString(path+"/"+dataset);
        ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("Loading %s ...",cmd.toString().c_str()));
        playerPort.write(cmd,rep);

        // connect ports opened up by yarpdataplayer
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/icub/camcalib/left/out","/yarp-opencv/image:i"),
                                  "Unable to connect dataset specific ports");

        cmd.clear();
        cmd.addString("play");
        playerPort.write(cmd,rep);

        return true;
    }

    /******************************************************************/
    virtual void tearDown()
    {
        yarp::os::Bottle cmd,rep;
        cmd.addString("stop");
        playerPort.write(cmd,rep);

        playerPort.close();
        targetPort.close();
    }

    /******************************************************************/
    virtual void run()
    {
        yarp::os::Time::delay(5.0);

        ROBOTTESTINGFRAMEWORK_TEST_REPORT("Checking target position in the image");
        int correct = 0;
        int errors  = 0;
        for (double t0=yarp::os::Time::now(); yarp::os::Time::now()-t0<15.0;)
        {
            yarp::os::Bottle *pTarget=targetPort.read(false);
            if (pTarget!=NULL)
            {
                double responseX = pTarget->get(0).asList()->get(0).asDouble();
                double responseY = pTarget->get(0).asList()->get(1).asDouble();


                if (responseX > 138 && responseX < 210 && responseY > 65 && responseY < 170)
                  correct++;
                else
                  errors++;

                //ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("RESPONSE IS : %lf %lf\n", responseX, responseY));
            }
        }
        ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("CORRECTNESS IS : %d %d\n", correct, errors));
        ROBOTTESTINGFRAMEWORK_TEST_CHECK( (correct > errors), "Checking passing of test");
    }
};

ROBOTTESTINGFRAMEWORK_PREPARE_PLUGIN(TestTutorialYarpOpencv)
