#include "sensors/ImuSynced.h"

ImuSynced::ImuSynced(ros::NodeHandle *nh, TimerSynced *timer)
    : SensorSynced(nh, timer) {}

void ImuSynced::setupRos(char *rate_topic, char *imu_topic) {
  static ros::Subscriber<std_msgs::UInt16, SensorSynced> rate_sub(
      rate_topic, &ImuSynced::changeRateCb, this);
  SensorSynced::setupRos(rate_sub);
  if (nh_) {
    // Create static ROS message.
    static sensor_msgs::Imu imu_msg;
    imu_msg_ = &imu_msg;

    // Create static ROS publisher.
    static ros::Publisher pub(imu_topic, imu_msg_);
    publisher_ = &pub;

    // Advertise.
    nh_->advertise(pub);
  }
}
