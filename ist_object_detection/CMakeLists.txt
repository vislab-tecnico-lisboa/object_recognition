cmake_minimum_required(VERSION 2.8.3)
project(ist_object_detection)

catkin_python_setup() 
## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS perception_msgs)

catkin_package(CATKIN_DEPENDS 
  perception_msgs
)



