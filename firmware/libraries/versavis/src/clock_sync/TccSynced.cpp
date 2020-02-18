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

  if (invert_trigger_) {
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

void TccSynced::setupExposure(const bool invert) const {
  DEBUG_PRINT("[TccSynced]: Configuring exposure pin ");
  DEBUG_PRINT(exposure_pin_.pin);
  DEBUG_PRINT(" of group ");
  DEBUG_PRINTLN(exposure_pin_.group);

  // Setup PORT.

  REG_PM_APBBMASK |= PM_APBBMASK_PORT; // Port ABP Clock Enable.
  // Configure as input.
  PORT->Group[mfrq_pin_.group].DIRCLR.reg |=
      PORT_DIRCLR_DIRCLR(1 << mfrq_pin_.pin);
  PORT->Group[mfrq_pin_.group].PINCFG[mfrq_pin_.pin].reg |= PORT_PINCFG_INEN;
  // Configure EXTINT[pin]
  if (exposure_pin_.pin % 2) {
    PORT->Group[exposure_pin_.group].PMUX[exposure_pin_.pin >> 1].reg |=
        PORT_PMUX_PMUXO_A;
  } else {
    PORT->Group[exposure_pin_.group].PMUX[exposure_pin_.pin >> 1].reg |=
        PORT_PMUX_PMUXE_A;
  }
  // Enable pin multiplexation.
  PORT->Group[exposure_pin_.group].PINCFG[exposure_pin_.pin].reg |=
      PORT_PINCFG_PMUXEN;

  // EVSYS (only configured for TCC timers)
  REG_PM_APBCMASK |= PM_APBCMASK_EVSYS;
  DEBUG_PRINTLN("[TimerSynced]: Configuring EVSYS exposure users.");
  // Channel 0 is used by RTC.
  EVSYS->USER.reg =
      EVSYS_USER_CHANNEL(3) | EVSYS_USER_USER(EVSYS_ID_USER_TCC1_MC_0);
  EVSYS->USER.reg =
      EVSYS_USER_CHANNEL(4) | EVSYS_USER_USER(EVSYS_ID_USER_TCC2_MC_0);

  DEBUG_PRINTLN("[TimerSynced]: Configuring EVSYS exposure channels.");

  // Setup interrupt
}

uint8_t TccSynced::getExposureEventGeneratorId() const {
  // https://github.com/ethz-asl/versavis_hw/blob/1e71a3843aefbbec8e6261c0855bd7cad7f38f9e/VersaVIS/bootloaders/mzero/Bootloader_D21/src/ASF/sam0/utils/cmsis/samd21/include/instance/evsys.h
  return (exposure_pin_.pin % 16) + 12;
}

void TccSynced::handleInterrupt() {
  if (tcc_->INTFLAG.bit.MC0 && (getWaveOutPinValue() ^ invert_trigger_)) {
    trigger();
  }
  if (tcc_->INTFLAG.bit.TRG) {
    syncRtc();
  } else if (tcc_->INTFLAG.bit.OVF) {
    overflow();
  }

  // Clear flags.
  tcc_->INTFLAG.reg |= tcc_->INTFLAG.bit.MC0;
  tcc_->INTFLAG.reg |= tcc_->INTFLAG.bit.TRG;
  tcc_->INTFLAG.reg |= tcc_->INTFLAG.bit.OVF;
}
