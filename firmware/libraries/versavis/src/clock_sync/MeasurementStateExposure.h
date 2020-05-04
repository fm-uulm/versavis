////////////////////////////////////////////////////////////////////////////////
//  March 2020
//  Author: Rik Bähnemann <brik@ethz.ch>
////////////////////////////////////////////////////////////////////////////////
//  MeasurementStateExposure.h
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

#ifndef Clock_Sync_MeasurementStateExposure_
#define Clock_Sync_MeasurementStateExposure_

#include <cstdint>
#include <ros/duration.h>
#include <ros/time.h>

#include "clock_sync/MeasurementStateStamped.h"
#include "clock_sync/atomic.h"

class MeasurementStateExposure : public MeasurementStateStamped {
public:
  inline void setStart(const ros::Time &time) { start_time_ = time; }
  inline void setEnd(const ros::Time &time) {
    auto half_exposure = time - start_time_;
    if (half_exposure.sec >= 0) {
      exposure_.sec = half_exposure.sec;
      exposure_.nsec = half_exposure.nsec;
      half_exposure *= 0.5;
      start_time_ += half_exposure;
      setTime(start_time_);
    }
  }

  inline bool getTime(ros::Time *time, uint32_t *num, ros::Duration *exp) {
    // Savely return state.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (MeasurementStateStamped::getTime(time, num)) {
        if (exp) {
          exp->sec = exposure_.sec;
          exp->nsec = exposure_.nsec;
        }
        return true;
      } else {
        return false;
      }
    }
  }

private:
  ros::Time start_time_;
  volatile ros::Duration exposure_;
};

#endif
