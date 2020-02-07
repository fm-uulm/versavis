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

  void setupMfrq(const uint16_t rate_hz, const bool invert) override;
  void setupDataReady(const uint8_t port_group, const uint8_t pin,
                      const InterruptLogic &logic) override;

  void handleInterrupt() override;

private:
  // Pointer to the actual timer.
  TcCount16 *tc_ = NULL;

  void setup() const;
};

#endif
