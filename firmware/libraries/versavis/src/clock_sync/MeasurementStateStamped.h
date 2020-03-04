////////////////////////////////////////////////////////////////////////////////
//  March 2020
//  Author: Rik Bähnemann <brik@ethz.ch>
////////////////////////////////////////////////////////////////////////////////
//  MeasurementStateStamped.h
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

#ifndef Clock_Sync_MeasurementStateStamped_
#define Clock_Sync_MeasurementStateStamped_

#include <cstdint>
#include <ros/time.h>

#include "clock_sync/MeasurementState.h"
#include "clock_sync/atomic.h"

class MeasurementStateStamped : public MeasurementState {
public:
  inline void setTime(const ros::Time &time) {
    setMeasurement();
    time_.sec = time.sec;
    time_.nsec = time.nsec;
  }

  inline bool getTime(ros::Time *time, uint32_t *num) {
    // Savely return state.
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (getDataReady(num)) {
        if (time) {
          time->sec = time_.sec;
          time->nsec = time_.nsec;
        }
        return true;
      } else {
        return false;
      }
    }
  }

private:
  volatile ros::Time time_;
};

#endif
