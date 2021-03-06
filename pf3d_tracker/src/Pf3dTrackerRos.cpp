#include "Pf3dTrackerRos.h"

Pf3dTrackerRos::Pf3dTrackerRos(const ros::NodeHandle & n_, const ros::NodeHandle & n_priv_) : n(n_), n_priv(n_priv_), it(n)
{

    camera_info_sub=n.subscribe("camera_info",1,&Pf3dTrackerRos::cameraInfoCallback, this);
}




void Pf3dTrackerRos::cameraInfoCallback(const sensor_msgs::CameraInfoPtr & camera_info)
{
    camera_info_sub.shutdown();

    int nParticles;
    double accelStdDev;
    double insideOutsideDifferenceWeight;
    double likelihoodThreshold;
    std::string models_folder;
    std::string trackedObjectColorTemplate;
    std::string trackedObjectColorTemplateFile;

    std::string trackedObjectShapeTemplate;
    std::string trackedObjectShapeTemplateFile;

    std::string motionModelMatrix;
    std::string motionModelMatrixFile;

    std::string dataFileName;
    std::string initializationMethod;
    double initialX;
    double initialY;
    double initialZ;

    //***********************************************
    // Read parameters from the initialization file *
    //***********************************************
    //Topics

    //Parameters for the algorithm
    n_priv.param<int>("nParticles", nParticles, 900); //TODO set it back to 900
    n_priv.param<double>("accelStdDev", accelStdDev, 30.0); //TODO set it back to 30
    n_priv.param<double>("insideOutsideDiffWeight", insideOutsideDifferenceWeight, 1.5);
    n_priv.param<double>("likelihoodThreshold", likelihoodThreshold, 0.005);//.005); TODO ser it back to 0.005
    n_priv.param<std::string>("models_folder", models_folder, "models_folder");
    n_priv.param<std::string>("trackedObjectColorTemplate", trackedObjectColorTemplate, "/home/vislab/repositories/ros/object_recognition/pf3dTracker/models/red_ball_iit.bmp");
    n_priv.param<std::string>("trackedObjectShapeTemplate", trackedObjectShapeTemplate, "/home/vislab/repositories/ros/object_recognition/pf3dTracker/models/initial_ball_points_smiley_31mm_20percent.csv");
    n_priv.param<std::string>("motionModelMatrix", motionModelMatrix, "/home/vislab/repositories/ros/object_recognition/pf3dTracker/models/motion_model_matrix.csv");
    n_priv.param<std::string>("trackedObjectTemp", dataFileName, "current_histogram.csv"); //might be removed? TODO
    n_priv.param<std::string>("initializationMethod", initializationMethod, "3dEstimate");
    n_priv.param<double>("initialX", initialX, 0.0);
    n_priv.param<double>("initialY", initialY, 0.0);
    n_priv.param<double>("initialZ", initialZ, 0.2);

    n_priv.param<bool>("crop_center", crop_center, false);



    //Camera intrinsic parameters
    trackedObjectColorTemplateFile=models_folder+trackedObjectColorTemplate;
    trackedObjectShapeTemplateFile=models_folder+trackedObjectShapeTemplate;
    motionModelMatrixFile=models_folder+motionModelMatrix;

    ROS_INFO_STREAM("crop_center: "<<crop_center);
    ROS_INFO_STREAM("nParticles: "<<nParticles);
    ROS_INFO_STREAM("accelStdDev: "<<accelStdDev);
    ROS_INFO_STREAM("insideOutsideDifferenceWeight: "<<insideOutsideDifferenceWeight);
    ROS_INFO_STREAM("likelihoodThreshold: "<<likelihoodThreshold);
    ROS_INFO_STREAM("trackedObjectColorTemplate: "<<trackedObjectColorTemplate);
    ROS_INFO_STREAM("trackedObjectShapeTemplate: "<<trackedObjectShapeTemplate);
    ROS_INFO_STREAM("motionModelMatrix: "<<motionModelMatrix);
    ROS_INFO_STREAM("initializationMethod: "<<initializationMethod);
    ROS_INFO_STREAM("initialX: "<<initialX);
    ROS_INFO_STREAM("initialY: "<<initialY);
    ROS_INFO_STREAM("initialZ: "<<initialZ);


    ROS_INFO("Getting cameras' parameters");

    //set the cameras intrinsic parameters
    cv::Mat cam_intrinsic = cv::Mat::eye(3,3,CV_64F);
    cam_intrinsic.at<double>(0,0) = camera_info->K.at(0);
    cam_intrinsic.at<double>(1,1) = camera_info->K.at(4);
    cam_intrinsic.at<double>(0,2) = camera_info->K.at(2);
    cam_intrinsic.at<double>(1,2) = camera_info->K.at(5);

    double width=(unsigned int)camera_info->width;
    double height=(unsigned int)camera_info->height;
    std::cout << cam_intrinsic << std::endl;
    std::cout << width << std::endl;
    std::cout << height << std::endl;
    //Units are meters outside the program, but millimeters inside, so we need to convert
    initialX*=1000.0;
    initialY*=1000.0;
    initialZ*=1000.0;
    tracker=boost::shared_ptr<PF3DTracker>(new PF3DTracker(nParticles,
                                                           accelStdDev,
                                                           insideOutsideDifferenceWeight,
                                                           likelihoodThreshold,
                                                           trackedObjectColorTemplateFile,
                                                           trackedObjectShapeTemplateFile,
                                                           motionModelMatrixFile,
                                                           dataFileName,
                                                           initializationMethod,
                                                           initialX,
                                                           initialY,
                                                           initialZ,
                                                           width,
                                                           height,
                                                           cam_intrinsic));

    // Set the input video port, with the associated callback method
    image_in = it.subscribe("image_in", 1, &Pf3dTrackerRos::processImageCallback, this);

    // Set the output ports
    image_out = it.advertise("image_out", 1);

    // Set the estimates out topic
    //estimates_out  = n.advertise<pf3d_tracker::Estimates>("estimates_out", 1);
    estimates_out  = n.advertise<geometry_msgs::PoseStamped>("estimates_out", 1);
}





void Pf3dTrackerRos::processImageCallback(const sensor_msgs::ImageConstPtr& msg_ptr)
{
    //int count;
    //unsigned int seed;
    //double likelihood, mean, maxX, maxY, maxZ;
    //double weightedMeanX, weightedMeanY, weightedMeanZ;
    //double meanU;
    //double meanV;
    //double wholeCycle;
    stringstream out;
    cv::Mat rawImageBGR;
    cv::Mat _rawImage;

    //seed=rand();
    bool _staticImageTest=false;
    if(_staticImageTest)
    {
        rawImageBGR = cv::imread( "testImage.png", 1) ;
        if( rawImageBGR.data == 0 ) //load the image from file.
        {
            std::cout << "Tried to open testImage.png"<<std::endl;
            cout<<"I wasn't able to open the test image file!\n";
            fflush(stdout);
            return; //if I can't do it, I just quit the program.
        }

        //_rawImage = cvCreateImage(cvSize(rawImageBGR.cols, rawImageBGR.rows), rawImageBGR->depth, rawImageBGR->nChannels);
        //cvCvtColor(rawImageBGR,_rawImage,CV_BGR2RGB);
        _rawImage=rawImageBGR;
    }
    else
    {
        //Read the image from the buffer
        try
        {
            std::string aux = "bgr8";

            _rawImage=cv_bridge::toCvShare(msg_ptr, aux)->image;
            _rawImage=cv_bridge::toCvCopy(msg_ptr, aux)->image;
            
        }
        catch (cv_bridge::Exception& e)
        {
            ROS_ERROR("cv_bridge exception: %s", e.what());
            return;
        }

        //_rawImage = bridge_.imgMsgToCv(msg_ptr,aux); //This is an RGB image
    }




    if(crop_center)
    {

        cv::Mat processed_img = cv::Mat::zeros(_rawImage.size(),  _rawImage.type());
        std::cout << _rawImage.size() << std::endl;
        cv::Mat mask = cv::Mat::zeros(_rawImage.size(), _rawImage.type());

        //cv::circle(mask, cv::Point(mask.cols/2, mask.rows/2), 300/2, cv::Scalar(255, 255, 255), -1, 8, 0);
        cv::rectangle(mask,cv::Point(mask.cols/2-300, mask.rows/2-300),cv::Point(mask.cols/2+300, mask.rows/2+300),cv::Scalar(255, 255, 255),-1,8,0);
        _rawImage.copyTo(processed_img, mask);
        _rawImage = processed_img;
    }

    tracker->processImage(_rawImage);


    /////////////////
    // DATA OUTPUT //
    /////////////////

    /*pf3d_tracker::Estimates outMsg;
    outMsg.mean.point.x=tracker->weightedMeanX/1000;
    outMsg.mean.point.y=tracker->weightedMeanY/1000;
    outMsg.mean.point.z=tracker->weightedMeanZ/1000;

    outMsg.likelihood=tracker->maxLikelihood/exp((double)20.0);
    outMsg.meanU=meanU;
    outMsg.meanV=meanV;
    //outMsg.seeingBall=_seeingObject;
    estimates_out.publish(outMsg);*/
    geometry_msgs::PoseStamped outMsg;
    outMsg.pose.position.x = tracker->weightedMeanX/1000;
    outMsg.pose.position.y = tracker->weightedMeanY/1000;
    outMsg.pose.position.z = tracker->weightedMeanZ/1000;
    outMsg.pose.orientation.x = 0.0;
    outMsg.pose.orientation.y = 0.0;
    outMsg.pose.orientation.z = 0.0;
    outMsg.pose.orientation.w = 1.0;
    outMsg.header.frame_id=msg_ptr->header.frame_id;
    estimates_out.publish(outMsg);


    sensor_msgs::ImagePtr img_msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", _rawImage).toImageMsg();

    image_out.publish(img_msg);

    if(_staticImageTest)
    {
        cv::imwrite("testImageOutput.png", _rawImage);
        ros::shutdown(); //exit the program after just one cycle
    }
}

int main(int argc, char *argv[])
{
    ros::init(argc, argv, "pf3d_tracker"); // set up ROS
    ros::NodeHandle n;
    ros::NodeHandle n_priv("~");

    Pf3dTrackerRos tracker(n,n_priv); //instantiate the tracker.
    ros::spin(); // pass control on to ROS

    return 0;
}
