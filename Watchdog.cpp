#include "Watchdog.h"

// Clock the WDT from the ultra-low-power 32 kHz oscillator divided by 32
// (1024 Hz) on generic clock generator 2, which the Arduino SAMD core leaves
// unused. PER = 16K cycles → 16 s.
void Watchdog::begin() {
  GCLK->GENDIV.reg = GCLK_GENDIV_ID(2) | GCLK_GENDIV_DIV(4);        // 2^(4+1) = 32
  while (GCLK->STATUS.bit.SYNCBUSY) {}
  GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_GENEN |
                      GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_DIVSEL;
  while (GCLK->STATUS.bit.SYNCBUSY) {}
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_WDT | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2;

  WDT->CTRL.reg = 0;                                               // disable while configuring
  while (WDT->STATUS.bit.SYNCBUSY) {}
  WDT->CONFIG.bit.PER = WDT_CONFIG_PER_16K_Val;
  WDT->CTRL.bit.ENABLE = 1;
  while (WDT->STATUS.bit.SYNCBUSY) {}
}

void Watchdog::feed() {
  WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
  while (WDT->STATUS.bit.SYNCBUSY) {}
}

bool Watchdog::causedLastReset() { return PM->RCAUSE.reg & PM_RCAUSE_WDT; }

const char *Watchdog::resetCause() {
  if (PM->RCAUSE.reg & PM_RCAUSE_WDT)  return "watchdog";
  if (PM->RCAUSE.bit.SYST) return "software";
  if (PM->RCAUSE.bit.EXT)  return "reset button";
  if (PM->RCAUSE.bit.BOD33 || PM->RCAUSE.bit.BOD12) return "brown-out";
  if (PM->RCAUSE.bit.POR)  return "power-on";
  return "unknown";
}
