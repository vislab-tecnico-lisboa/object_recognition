#! /usr/bin/env python

import roslib; #roslib.load_manifest('ist_tasks')
import rospy
import actionlib
from perception_msgs.msg import *
from vizzy_msgs.msg import *
from tabletop_object_segmentation_online.srv import *
import numpy
import math
from array import array
import time



def perception_client():
	# Region bounding the plannar surface and the objects of interest 
	table_region=tabletop_object_segmentation_online.msg.TableRegion() 

	#UPPER SHELF
  	table_region.x_filter_max=1.0
  	table_region.x_filter_min=-1.0
  	table_region.y_filter_max=1
  	table_region.y_filter_min=-1
  	table_region.z_filter_max=3.0
  	table_region.z_filter_min=0.1

	

	# Creates the SimpleActionClient, passing the type of the action (DetectObjectsAction) to the constructor.
	client = actionlib.SimpleActionClient('detect_objects_server', perception_msgs.msg.DetectObjectsAction)

	# Waits until the action server has started up and started listening for goals.
	client.wait_for_server()
	goal = perception_msgs.msg.DetectObjectsGoal()
	goal.table_region = table_region
	client.send_goal(goal)#, self.perception_done_cb, self.perception_active_cb, self.perception_feedback_cb)
	#client.cancel_goal()
	client.get_state()

	# Waits for the server to finish performing the action.
	client.wait_for_result()
	if client.get_result()==False:
		print 'Detected 0 objects.\n'


if __name__ == '__main__':
    try:
        # Initializes a rospy node so that the SimpleActionClient can
        # publish and subscribe over ROS.
        rospy.init_node('detect_object_clusters_client')

        # FIRST LOOK DOWN
	# Creates the SimpleActionClient, passing the type of the action (DetectObjectsAction) to the constructor.
	client = actionlib.SimpleActionClient('gaze', vizzy_msgs.msg.GazeAction)

	# Waits until the action server has started up and started listening for goals.
	client.wait_for_server()
	goal = vizzy_msgs.msg.GazeGoal()
        goal.type = 0 # vizzy_msgs.GazeGoal.CARTESIAN
        goal.fixation_point.point.x = 1.0;
        goal.fixation_point.point.y = 0.0;
        goal.fixation_point.point.z = 0.2;
        goal.fixation_point_error_tolerance = 0.001;
	goal.fixation_point.header.frame_id="base_footprint";
	client.send_goal(goal)#, self.perception_done_cb, self.perception_active_cb, self.perception_feedback_cb)
	#client.cancel_goal()
	client.get_state()

	# Waits for the server to finish performing the action.
	print 'wait for gaze server to finish'
	client.wait_for_result()
	print 'done'


	#while rospy.is_shutdown()==False:
        perception_client()
    except rospy.ROSInterruptException:
        print "program interrupted before completion"
