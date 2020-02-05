////////////////////////////////////////////////////////////////////////////////
//  January 2020
//  Author: Rik Bähnemann <brik@ethz.ch>
////////////////////////////////////////////////////////////////////////////////
//  TcSynced.h
////////////////////////////////////////////////////////////////////////////////

#ifndef TcSynced_h
#define TcSynced_h

#include <Arduino.h>

#include "clock_sync/TimerSynced.h"

class TcSynced : public TimerSynced {
public:
  TcSynced(TcCount16 *tc);

  void setup() const override;

  void handleRetrigger() override;
  void handleOverflow() override;

private:
  // Pointer to the actual timer.
  TcCount16 *tc_ = NULL;
};

#endif
