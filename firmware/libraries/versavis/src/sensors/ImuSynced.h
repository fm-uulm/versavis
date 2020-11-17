////////////////////////////////////////////////////////////////////////////////
//  February 2020
//  Author: Rik Bähnemann <brik@ethz.ch>
////////////////////////////////////////////////////////////////////////////////
//  ImuSynced.h
////////////////////////////////////////////////////////////////////////////////
//
//  Basic implementation for IMUs in the versavis framework. Refer to
//  the parent package versavis for license information.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef Sensors_ImuSynced_h
#define Sensors_ImuSynced_h

#include "sensors/SensorSynced.h"
#include <versavis/ImuMicro.h>

class ImuSynced : public SensorSynced {
public:
  ImuSynced(ros::NodeHandle *nh, TimerSynced *timer);
  void setupRos(char *rate_topic, char *imu_topic);

protected:
  versavis::ImuMicro *imu_msg_ = NULL;
};

#endif
