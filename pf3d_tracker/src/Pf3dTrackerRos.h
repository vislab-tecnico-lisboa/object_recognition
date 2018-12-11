#ifndef PF3DTRACKERROS_H
#define PF3DTRACKERROS_H
#include <ros/ros.h>
#include "image_transport/image_transport.h"
#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>
//#include <cv_bridge/CvBridge.h>
#include <cv_bridge/cv_bridge.h>
#include <geometry_msgs/PoseStamped.h>
#include <std_msgs/Time.h>
#include <pf3d_tracker/Estimates.h>
#include <iCub/pf3dTracker.hpp>

using namespace cv_bridge;

class Pf3dTrackerRos
{

    ros::NodeHandle n;
    ros::NodeHandle n_priv;
    image_transport::ImageTransport it;
    ros::Publisher _outputUVDataPort;
    image_transport::Subscriber  image_in;
    image_transport::Publisher image_out;
    ros::Publisher estimates_out;

    ros::Subscriber camera_info_sub;
    std_msgs::Time _rosTimestamp;

    cv_bridge::CvImageConstPtr _rosImage;

    bool crop_center;


    //std::string _inputParticlePortName;
    //BufferedPort<Bottle> _inputParticlePort;
    std::string _outputParticlePortName;
    //BufferedPort<Bottle> _outputParticlePort;
    //std::string _outputAttentionPortName;
    //ros::Publisher _outputAttentionPort;
    std::string _outputUVDataPortName;
    double image_ratio;

    boost::shared_ptr<PF3DTracker> tracker;
public:
    Pf3dTrackerRos(const ros::NodeHandle & n_, const ros::NodeHandle & n_priv_);
    void processImageCallback(const sensor_msgs::ImageConstPtr& msg_ptr);


    // initialization callback
    void cameraInfoCallback(const sensor_msgs::CameraInfoPtr & camera_info);

};

#endif // PF3DTRACKERROS_H
