#include "clock_sync/TccSynced.h"

#include "helper.h"

TccSynced::TccSynced(const MfrqPin &mfrq_pin, const ExposurePin &exp_pin,
                     Tcc *tcc)
    : TimerSynced(mfrq_pin), exposure_pin_(exp_pin), tcc_(tcc) {
  setup();
}

void TccSynced::setup() const {
  if (!tcc_) {
    error("NO_TIMER (TccSynced.cpp): timer does not exist.", 201);
  }

  DEBUG_PRINTLN("[TccSynced]: Disabling timer.");
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }
  tcc_->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }

  DEBUG_PRINTLN("[TccSynced]: Setup EVCTRL to retrigger on RTC overflow.");
  tcc_->EVCTRL.reg |= TCC_EVCTRL_TCEI0 | TCC_EVCTRL_EVACT0_RETRIGGER;

  DEBUG_PRINTLN("[TccSynced]: Enabling event interrupts.");
  tcc_->INTENSET.reg |= TCC_INTENSET_TRG | TCC_INTENSET_OVF;
  DEBUG_PRINTLN("[TccSynced]: Clearing interrupt flags.");
  tcc_->INTFLAG.reg |= TCC_INTFLAG_TRG | TCC_INTFLAG_OVF;

  DEBUG_PRINTLN("[TccSynced]: Enable timer.");
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }
  tcc_->CTRLA.reg |= TCC_CTRLA_ENABLE;
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }
}

void TccSynced::setupMfrqWaveform() const {
  // Setup wavegen.
  DEBUG_PRINTLN("[TccSynced]: Disabling timer.");
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }
  tcc_->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }

  DEBUG_PRINT("[TccSynced]: Prescaling timer by ");
  DEBUG_PRINTLN(prescaler_);
  tcc_->CTRLA.reg |= TCC_CTRLA_PRESCALER(prescaler_);
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }

  DEBUG_PRINTLN("[TccSynced]: Activate MFRQ.");
  tcc_->WAVE.reg |= TCC_WAVE_WAVEGEN_MFRQ;
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }

  DEBUG_PRINTLN("[TccSynced]: Make CC0 compare register.");
  tcc_->CTRLA.reg &= ~TCC_CTRLA_CPTEN0;
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }

  // Negate to emit the first pulse at the first tick.
  if (!trigger_state_.invert_) {
    tcc_->DRVCTRL.reg |= TCC_DRVCTRL_INVEN0;
    while (tcc_->SYNCBUSY.bit.ENABLE) {
    }
  }

  DEBUG_PRINT("[TccSynced]: Set FRQ top: ");
  DEBUG_PRINTLN(top_);
  while (tcc_->SYNCBUSY.bit.CC0) {
  }
  tcc_->CC[0].reg = top_;
  while (tcc_->SYNCBUSY.bit.CC0) {
  }

  DEBUG_PRINTLN("[TccSynced]: Enabling MFRQ interrupts.");
  tcc_->INTENSET.reg |= TCC_INTENSET_MC0;
  tcc_->INTFLAG.reg |= TCC_INTFLAG_MC0;

  DEBUG_PRINTLN("[TccSynced]: Enable timer.");
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }
  tcc_->CTRLA.reg |= TCC_CTRLA_ENABLE;
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }
}

void TccSynced::setupExposure(const bool invert) {
  DEBUG_PRINT("[TccSynced]: Configuring exposure pin ");
  DEBUG_PRINT(exposure_pin_.pin);
  DEBUG_PRINT(" of group ");
  DEBUG_PRINTLN(exposure_pin_.group);

  exposure_state_.invert_ = invert;

  DEBUG_PRINTLN("[TccSynced]: Setup interrupt pin.");
  setupInterruptPin(exposure_pin_.group, exposure_pin_.pin,
                    InterruptLogic::kBoth, false);
  DEBUG_PRINTLN("[TccSynced]: Setup exposure evsys.");
  setupExposureEvsys();

  // Setup interrupt
  DEBUG_PRINTLN("[TccSynced]: Disabling timer.");
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }
  tcc_->CTRLA.reg &= ~TCC_CTRLA_ENABLE;
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }
  DEBUG_PRINTLN("[TccSynced]: Configure exposure capture and interrupt.");
  tcc_->EVCTRL.reg |= TCC_EVCTRL_MCEI1;
  DEBUG_PRINTLN("[TccSynced]: TCC_CTRLA_CPTEN1.");
  tcc_->CTRLA.reg |= TCC_CTRLA_CPTEN1;
  DEBUG_PRINTLN("[TccSynced]: TCC_INTENSET_MC1.");
  tcc_->INTENSET.reg |= TCC_INTENSET_MC1;
  DEBUG_PRINTLN("[TccSynced]: TCC_INTFLAG_MC1.");
  tcc_->INTFLAG.reg |= TCC_INTFLAG_MC1;
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }

  DEBUG_PRINTLN("[TccSynced]: Enable timer.");
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }
  tcc_->CTRLA.reg |= TCC_CTRLA_ENABLE;
  while (tcc_->SYNCBUSY.bit.ENABLE) {
  }
}

uint8_t TccSynced::getEventGeneratorId(const uint8_t pin) const {
  // https://github.com/ethz-asl/versavis_hw/blob/1e71a3843aefbbec8e6261c0855bd7cad7f38f9e/VersaVIS/bootloaders/mzero/Bootloader_D21/src/ASF/sam0/utils/cmsis/samd21/include/instance/evsys.h
  return (pin % 16) + 12;
}

bool TccSynced::getExposurePinValue() const {
  return getPinValue(exposure_pin_.group, exposure_pin_.pin);
}

void TccSynced::handleInterrupt() {
  // Handle RTC retrigger.
  if (tcc_->INTFLAG.bit.TRG) {
    trigger_state_.syncRtc();
    exposure_state_.syncRtc();
    pps_state_.syncRtc();
    tcc_->INTFLAG.reg |= tcc_->INTFLAG.bit.TRG;
    tcc_->INTFLAG.reg |= tcc_->INTFLAG.bit.OVF;
  } else if (tcc_->INTFLAG.bit.OVF) {
    trigger_state_.overflow();
    exposure_state_.overflow();
    pps_state_.overflow();
    tcc_->INTFLAG.reg |= tcc_->INTFLAG.bit.OVF;
  }

  // Handle wave generator trigger.
  if (tcc_->INTFLAG.bit.MC0 &&
      (getWaveOutPinValue() ^ trigger_state_.invert_)) {
    trigger_state_.trigger(prescaler_, top_);
    tcc_->INTFLAG.reg |= tcc_->INTFLAG.bit.MC0;
  }

  // Handle exposure.
  if (tcc_->INTFLAG.bit.MC1 &&
      (getExposurePinValue() ^ exposure_state_.invert_)) {
    exposure_state_.startExposure(tcc_->CC[1].reg, prescaler_, top_);
    tcc_->INTFLAG.reg |= tcc_->INTFLAG.bit.MC1;
  } else if (tcc_->INTFLAG.bit.MC1) {
    exposure_state_.stopExposure(tcc_->CC[1].reg, prescaler_, top_);
    tcc_->INTFLAG.reg |= tcc_->INTFLAG.bit.MC1;
  }

  // Handle PPS interrupt.
  if (tcc_->INTFLAG.bit.MC2) {
    pps_state_.receive(tcc_->CC[2].reg, prescaler_, top_);
    tcc_->INTFLAG.reg |= tcc_->INTFLAG.bit.MC2;
  }
}
