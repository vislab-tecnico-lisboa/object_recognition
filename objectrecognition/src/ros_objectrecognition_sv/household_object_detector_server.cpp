#include "ros/ros.h"
#include <ros_objectrecognition/object_recognition_ros.h>
#include <tabletop_object_segmentation_online/TabletopSegmentation.h>
//#include <pcl/ros/conversions.h>
#include <sensor_msgs/point_cloud_conversion.h>
#include <pcl_ros/point_cloud.h>
#include <pcl/point_types.h>
#include <perception_msgs/PoseEstimation.h>
#include <boost/bind.hpp>

#include  <tf/transform_listener.h>

#include "objectrecognition_sv/object_model.h"
#include "objectrecognition_sv/pose_estimation.h"
#include "objectrecognition/models.h"
ros::Publisher pub; 
template <class objectModelT>
ros::Publisher objectRecognitionRos<objectModelT>::marker_pub;

typedef std::vector< std::vector<cluster> > Hypotheses;
typedef boost::function<bool(perception_msgs::PoseEstimation::Request&, perception_msgs::PoseEstimation::Response&)> recogntion_pose_estimation_callback;

bool recogntion_pose_estimation(perception_msgs::PoseEstimation::Request  &req,
				perception_msgs::PoseEstimation::Response &res,
				ros::NodeHandle & n,
				bottomup_msgs::bottomup_msg msg_bottomup,
				const models<objectModelSV> & models_library,
				std::vector< boost::shared_ptr<poseEstimationSV> > &poseEstimators,
				const int & angle_bins,
				const std::string & processing_frame,
				const int & hypotheses_per_model)
{
  	tf::TransformListener listener;
   	pcl::PointXYZ min_pt, max_pt;	
   	Hypotheses hypotheses;
	std::vector<pcl::PointCloud<pcl::PointNormal>::Ptr> clouds;
   	int bestModelIndex;
   	int bestHypothesisIndex;
   	float bestHypothesisVotes;	
	int region_id=0;
	if(req.cluster_list.size()==0)
	{	
		ROS_INFO("No detections returned by tabletop object detector node.");
		return false;
	}

	BOOST_FOREACH (const sensor_msgs::PointCloud& clus, req.cluster_list)
	{

		msg_bottomup.header.seq=req.cluster_list[0].header.seq; // BUG AQUI: RESOLVER ISTO
		msg_bottomup.header.stamp=req.cluster_list[0].header.stamp;
		msg_bottomup.header.frame_id=processing_frame;
		msg_bottomup.table=req.table;
		ROS_INFO("FRAME_TABLE:%f,%f,%f", req.table.pose.pose.position.x,req.table.pose.pose.position.y,req.table.pose.pose.position.z);

		cout << endl;
		cout << endl;
		cout << endl;
    		sensor_msgs::PointCloud clusterProcessingFrame;
		pcl::PointCloud<pcl::PointXYZ>::Ptr sceneClusterCloud(new pcl::PointCloud<pcl::PointXYZ>);

		/////////////////////////////////
		// convert to processing frame //
		/////////////////////////////////

		try
		{
			listener.transformPointCloud(processing_frame, clus, clusterProcessingFrame); 
			ROS_INFO("Cluster point cloud transformed from frame %s into frame %s", clus.header.frame_id.c_str(), processing_frame.c_str()); 
		}
		catch (tf::TransformException &ex)
		{
			ROS_ERROR("Failed to transform cloud from frame %s into frame %s", clus.header.frame_id.c_str(), 
			processing_frame.c_str());

			return -1;
		}

		sensor_msgs::PointCloud2 convertedCloud;
		sensor_msgs::convertPointCloudToPointCloud2(clusterProcessingFrame,convertedCloud); // this cloud is streamed in the output message

  		pcl::fromROSMsg(convertedCloud, *sceneClusterCloud);

		for(unsigned int j=0;j<poseEstimators.size(); ++j)								
		{
			//ROS_INFO("Compute hypothesis for model %d...", j);
			hypotheses.push_back(poseEstimators[j]->poseEstimationCore(sceneClusterCloud));
			//ROS_INFO("HYPOTHESIS NUMBER FOR MODEL %d: %d",j,hypotheses[j].size());
			ROS_INFO("Done");
		}

		/////////////////////////
		// Fill region message //
		/////////////////////////

		// Get cluster region bounding box
		pcl::getMinMax3D(*sceneClusterCloud, min_pt, max_pt);
		ROS_INFO("_minimumPoint.x: %f_maximumPoint.x: %f _minimumPoint.y: %f _maximumPoint.y: %f _minimumPoint.z: %f _maximumPoint.z: %f",min_pt.x, max_pt.x, min_pt.y, max_pt.y, min_pt.z, max_pt.z);


		////////////////////////////////////				
		// Choose cluster with more votes //
		////////////////////////////////////

		bestModelIndex=0;
		bestHypothesisIndex=0;
		bestHypothesisVotes=0;

		for(size_t m = 0; m < hypotheses.size(); ++m)
		{
			if(hypotheses[m][0].meanPose.votes>bestHypothesisVotes)
			{
				bestHypothesisVotes=hypotheses[m][0].meanPose.votes;
				bestModelIndex=m;
				//bestHypothesisIndex=0;
			}
		}

		if(bestHypothesisVotes==0)
		{	
			ROS_INFO("No hypotheses!");
			hypotheses.clear();
			continue;
		}

		// Store region message
		msg_bottomup.Region.push_back(objectRecognitionRos<objectModelSV>::fillRegionMsg<cluster>(region_id++, hypotheses, models_library, hypotheses_per_model, angle_bins, min_pt, max_pt, processing_frame));

		// Transform model cloud
		pcl::PointCloud<pcl::PointNormal>::Ptr cloudOut(new pcl::PointCloud<pcl::PointNormal>);
		ROS_INFO("Best hypothesis: modelIndex: %d clusterIndex:%d votes: %f", bestModelIndex, bestHypothesisIndex, hypotheses[bestModelIndex][bestHypothesisIndex].meanPose.votes);
		pcl::transformPointCloud(*(poseEstimators[bestModelIndex]->model->modelCloud), *cloudOut, hypotheses[bestModelIndex][bestHypothesisIndex].meanPose.getTransformation());

		clouds.push_back(cloudOut);

		hypotheses.clear();

	}

	///////////////////////
	// Visualize results //
	///////////////////////

	if(!clouds.empty())
	{
		ROS_INFO("CLOUDS NOT EMPTY");
		//pub = n.advertise<sensor_msgs::PointCloud2> ("output", 1);
		//sensor_msgs::PointCloud2 pc2;
		//pcl::toROSMsg(*clouds[0],pc2);

		//pcl_cloud.header = pcl_conversions::toPCL(pc2.header);
		//pub.publish (pc2);
		objectRecognitionRos<objectModelSV>::visualize<pcl::PointNormal>(clouds,n,processing_frame,2,2,"detection",0.01,1.0);

		//objectRecognitionRos<objectModelSV>::visualize<pcl::PointNormal>(graspablePointsClouds,n,_processing_frame,1,1,"graspable_points",0.01,1.0);
	}
	res.object_list=msg_bottomup;
  return true;
}

int main(int argc, char **argv)
{
	ros::init(argc, argv, "recognition_pose_estimation_server");
  	ros::NodeHandle n_priv("~");
  	ros::NodeHandle n;

  	////////////////////////////
  	// Data in/out parameters //
	////////////////////////////

  	// Working frame
	std::string processing_frame;
   	n_priv.param<std::string>("processing_frame", processing_frame, "/openni_rgb_optical_frame");
   	ROS_INFO("processing_frame: %s", processing_frame.c_str());

   	// Models Descriptors library path
	std::string models_database_path;
   	n_priv.param<std::string>("models_database_path", models_database_path, "/home/");
   	ROS_INFO("models_database_path: %s", models_database_path.c_str());

   	// Models Descriptors library file
	std::string models_database_file;
   	n_priv.param<std::string>("models_database_file", models_database_file, "file.test");
   	ROS_INFO("models_database_file: %s", models_database_file.c_str());

   	//Models specification path
	std::string models_specification_path;
   	n_priv.param<std::string>("models_specification_path", models_specification_path, "/filepath/");
   	ROS_INFO("models_specification_path: %s", models_specification_path.c_str());
	
   	// Models info xml parser
	std::string models_specification_file;
   	n_priv.param<std::string>("models_specification_file", models_specification_file, "file.xml");
   	ROS_INFO("models_specification_file: %s", models_specification_file.c_str());

	////////////////////////////
	// Operational parameters //
	////////////////////////////

	int angle_bins;
	n_priv.param<int>("angle_bins", angle_bins, 15);
	int distanceBins;
	n_priv.param<int>("distance_bins", distanceBins, 20);
	double referencePointsPercentage;
	n_priv.param<double>("reference_points_percentage",referencePointsPercentage, 0.50);
	double pre_downsampling_step;
	n_priv.param<double>("pre_downsampling_step",pre_downsampling_step, 0);
	double accumulatorPeakThreshold;
	n_priv.param<double>("accumulatorPeakThreshold",accumulatorPeakThreshold, 0.65);
	double radius;
	n_priv.param<double>("radius",radius, 0.04);
	double neighbours;
	n_priv.param<double>("neighbours",neighbours, 50);
	bool radius_search;
	n_priv.param<bool>("radius_search",radius_search, true);
	bool filter_on;
	n_priv.param<bool>("filter_on",filter_on, false);
	int hypotheses_per_model;
  	n_priv.param<int>("hypotheses_per_model", hypotheses_per_model,200);
	/////////////////////////
   	// Database parameters //
	/////////////////////////

	std::string database_name;
	n_priv.param<std::string>("database_name", database_name, "household_objects-0.2");
	std::string database_host;
	n_priv.param<std::string>("database_host", database_host, "localhost");
	std::string database_port;
	n_priv.param<std::string>("database_port", database_port, "5432");
	std::string database_user;
	n_priv.param<std::string>("database_user", database_user, "willow");
	std::string database_pass;
	n_priv.param<std::string>("database_pass", database_pass, "willow");

	///////////
	// train //
	///////////

	// Change models parameters
	objectModelSV(angle_bins,distanceBins,radius,neighbours,radius_search);

	// Create models object
	models<objectModelSV> models_library;
	//std::cout << " FILE:" << (models_database_path+models_database_file) << std::endl;
	// Load models on the household objects database, to the "models_library" object
	models_library.loadModels(true, 1.0, n, database_name, database_host, database_port, database_user, database_pass, (models_specification_path+models_specification_file), (models_database_path+models_database_file), processing_frame);

	// Change pose estimation parameters
	poseEstimation(referencePointsPercentage,accumulatorPeakThreshold,filter_on);

	// Create pose estimation objects
	std::vector< boost::shared_ptr<poseEstimationSV> > poseEstimators;
	for(size_t i=0; i < models_library.objectModels.size(); i++)
		poseEstimators.push_back(boost::shared_ptr<poseEstimationSV> (new poseEstimationSV(models_library.objectModels[i]) ) );
	
	bottomup_msgs::bottomup_msg msg_bottomup;	
	msg_bottomup.hypotheses_per_object=hypotheses_per_model;
	msg_bottomup.db_objects=models_library.objectModels.size();
	msg_bottomup.db_poses=angle_bins*angle_bins*angle_bins;

	objectRecognitionRos<objectModelSV>::marker_pub = n.advertise<visualization_msgs::Marker>("detector_markers_out", 1);


  	ros::ServiceServer service = n.advertiseService<perception_msgs::PoseEstimation::Request,perception_msgs::PoseEstimation::Response>("object_recognition_pose_estimation",boost::bind(&recogntion_pose_estimation, _1,_2,n_priv, msg_bottomup, models_library, poseEstimators, angle_bins, processing_frame, hypotheses_per_model));

 
  	ROS_INFO("Ready to do object recognition and pose estimation.");

  	ros::spin();

  	return 0;
}
