/*******************************************************************************

    ZeroPowerManager - Support for very low power Arduino Zero operation.

    Only the output of generic clock generator 0 (GCLK_MAIN) is changed.
    This will affect anything running off of that generic clock. For
    basic Arduino sketches this is only the CPU clock and the SysTick
    timer, which drives the delay() counter. So delay() can still be used
    but must be scaled proportionately to the CPU clock from the default
    48 MHz.

    According to the data sheet, the oscillator power consumption is:
    400 uA  DFLL48M
     64 uA  OSC8M
      2 uA  XOSC32K

    The DFLL48M is disabled when not being used as the CPU clock since it
    consumes substantial power. The OSC8M and XOSC32K are never disabled.

    Measured power consumption in a delay() loop:
        48 MHz (DFLL48M): 7.9 mA
         8 MHz (OSC8M)  : 1.4 mA
         1 MHz (OSC8M/8): 1.4 mA    // not implemented, no improvement over 8 MHz
        32 KHz (XOSC32K): 200 uA
 
    On startup, SystemInit() configures the system clocks as follows:
    1) Enable XOSC32K clock (External on-board 32.768Hz oscillator), will be used as DFLL48M reference.
    2) Put XOSC32K as source of Generic Clock Generator 1
    3) Put Generic Clock Generator 1 as source for Generic Clock Multiplexer 0 (DFLL48M reference)
    4) Enable DFLL48M clock
    5) Switch Generic Clock Generator 0 to DFLL48M. CPU will run at 48MHz.
    6) Modify PRESCaler value of OSCM to have 8MHz
    7) Put OSC8M as source for Generic Clock Generator 3

    When the processor is put to sleep, the CPU clock will stop and SysTick
    stops and so SysTick cannot be used for timing. The RTC can be configured
    to count XOSC32K ticks to keep time through sleep events and to wake up
    the processor from sleep. The RTC is driven off XOSC32K with a divide-by-32
    prescaler resulting in approximately 1 ms ticks. More precisely, the
    timer tracks seconds in a 22.10 fractional format. The counter will
    roll over approximately every 48 days.

    COPYRIGHT (c) 2018 ee-quipment.com

 ******************************************************************************/


#include  <stdint.h>
#include  <samd.h>
#include  <Arduino.h>
#include  "ZeroPowerManager.h"


/******************************************************************************/
/**************************** PORT CONFIGURATION ******************************
 *
 * Disable unneeded ports so they won't oscillate and consume power.
 *
 ******************************************************************************/

/*******************************************************************************

    void  zpmPortDisableDigital(void)

    Set all Arduino digital ports to their power-up disabled state.

******************************************************************************/
void  zpmPortDisableDigital(void) {
    for (uint32_t ul=0; ul<NUM_DIGITAL_PINS; ul++) {
        PORT->Group[g_APinDescription[ul].ulPort].PINCFG[g_APinDescription[ul].ulPin].reg=0 ;
    }
}


/*******************************************************************************

    void  zpmPortDisableSPI(void)

    Set the default Arduino SPI pins to their power-up disabled state.

******************************************************************************/
void  zpmPortDisableSPI(void) {
    for (uint32_t ul=PIN_SPI_MISO; ul<=PIN_SPI_SCK; ul++) {
        PORT->Group[g_APinDescription[ul].ulPort].PINCFG[g_APinDescription[ul].ulPin].reg=0 ;
    }
}


/*******************************************************************************

    void  zpmPortDisableUSB(void)

    Completely turn off the USB module.

******************************************************************************/
void  zpmPortDisableUSB(void) {
    REG_USB_CTRLA = 0;
}


/******************************************************************************/
/************************** CPU CLOCK CONFIGURATION ****************************
 *
 * Set the CPU clock speed by configuring the output of generic clock
 * generator 0 (GCLK_MAIN).
 *
 ******************************************************************************/

/*******************************************************************************

    void _configGCLK0(uint8_t source_clock)

    Set Generic Clock Generator 0 to source_clock.


******************************************************************************/
void _configGCLK0(uint8_t source_clock) {

   GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(0)                     |
                       (source_clock << GCLK_GENCTRL_SRC_Pos) |
                       GCLK_GENCTRL_IDC                       |
                       GCLK_GENCTRL_GENEN ;
}


/*******************************************************************************

    void _configGCLK_MAIN(uint8_t source_clock)

    Configure the CPU clock from the low frequency source_clk. Set flash
    wait states to 0 and disable DFLL48M.

******************************************************************************/
void _configGCLK_MAIN(uint8_t source_clock) {
    _configGCLK0(source_clock);
    NVMCTRL->CTRLB.bit.RWS = NVMCTRL_CTRLB_RWS_SINGLE_Val;  // flash wait states = 0
    SYSCTRL->DFLLCTRL.reg  = 0;                             // disable DFLL48M
}


/*******************************************************************************

    void zpmClk8M(void)

    Source Generic Clock Generator 0 from OSC8M.
    The CPU will now run at 8 MHz.
    Set flash wait-states to 0.
    Leave XOSC32K as source of Generic Clock Generator 1.
    Leave OSC8M as the source for Generic Clock Generator 3.
    Disable DFLL48M.

******************************************************************************/
void zpmCPUClk8M(void) {
    _configGCLK_MAIN(GCLK_SOURCE_OSC8M);
}


/*******************************************************************************

    void zpmClk32K(void)

    Source Generic Clock Generator 0 from XOSC32K.
    The CPU will now run at 32 KHz.
    Set flash wait-states to 0.
    Leave XOSC32K as source of Generic Clock Generator 1.
    Leave OSC8M as the source for Generic Clock Generator 3.
    Disable DFLL48M.

******************************************************************************/
void zpmCPUClk32K(void) {
    _configGCLK_MAIN(GCLK_SOURCE_XOSC32K);
}


/*******************************************************************************

    void zpmCPUClk48M(void)

    Restores the oscillators and generic clock generators to their Arduino
    startup state. Copied from SystemInit() in startup.c.

******************************************************************************/
void zpmCPUClk48M(void) {
    NVMCTRL->CTRLB.bit.RWS = NVMCTRL_CTRLB_RWS_HALF_Val;  // flash wait states = 1

   // put Generic Clock Generator 1 as source for Generic Clock Multiplexer 0 (DFLL48M reference)
   GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(0)     | // Generic Clock Multiplexer 0
                       GCLK_CLKCTRL_GEN_GCLK1 | // Generic Clock Generator 1 is source
                       GCLK_CLKCTRL_CLKEN ;
   while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

   // remove the OnDemand mode, Bug http://avr32.icgroup.norway.atmel.com/bugzilla/show_bug.cgi?id=9905
   SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;

   while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0);

   SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_CSTEP(31)  | // Coarse step is 31, half of the max value
                          SYSCTRL_DFLLMUL_FSTEP(511) | // Fine step is 511, half of the max value
                          SYSCTRL_DFLLMUL_MUL(((48000000ul)/(32768ul))) ; // External 32KHz is the reference

   while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0);

   // write full configuration to DFLL control register
   SYSCTRL->DFLLCTRL.reg |= SYSCTRL_DFLLCTRL_MODE | /* Enable the closed loop mode */
                            SYSCTRL_DFLLCTRL_WAITLOCK |
                            SYSCTRL_DFLLCTRL_QLDIS ; /* Disable Quick lock */

   while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0);

   // enable the DFLL
   SYSCTRL->DFLLCTRL.reg |= SYSCTRL_DFLLCTRL_ENABLE ;

   while (((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLLCKC) == 0) ||
           ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLLCKF) == 0)); // wait for locks flags

   while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0);

   // switch Generic Clock Generator 0 to DFLL48M. CPU will run at 48MHz.
   GCLK->GENDIV.reg = GCLK_GENDIV_ID(0) ; // Generic Clock Generator 0

   while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

   // write Generic Clock Generator 0 configuration
   GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(0)       |
                       GCLK_GENCTRL_SRC_DFLL48M | // Selected source is DFLL 48MHz
                       GCLK_GENCTRL_IDC         | // Set 50/50 duty cycle
                       GCLK_GENCTRL_GENEN ;

   while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
}


/******************************************************************************/
/****************************** CLOCK FUNCTIONS *******************************
 *
 * The RTC counts ticks from XOSC32K instead of 1 second tics. This provides
 * a stable millisecond resolution clock in the absence of a stable SysTick,
 * allowing the CPU clock (which drives SysTick) to be varied or stopped.
 *
 ******************************************************************************/

static volatile uint32_t     _g_RTC_interrupt_interval = 0;    // for periodic interrupts
static volatile voidFuncPtr  _g_RTC_callBack = NULL;           // RTC interrupt user handler
static volatile bool         _g_f_playing_possum = false;      // flag set by ISR to release spin loop


/*******************************************************************************

    void zpmRTCInit(void)

    Set up the RTC to run continuously from XOSC32K in 32 bit counter mode
    with a divide-by-32 prescaler. The count register represents seconds
    in 22.10 fractional format.

    The Arduino system has already configured XOSC32K to be running and
    driving generic clock generator 1, but it must be configured to keep
    running in standby.

    A software reset of the RTC is done on initialization to ensure that
    the module is reset, since only a POR resets the RTC.

******************************************************************************/
void zpmRTCInit(void) {

      // keep the XOSC32K running in standy
      SYSCTRL->XOSC32K.reg |= SYSCTRL_XOSC32K_RUNSTDBY;

      // attach GCLK_RTC to generic clock generator 1
      GCLK->CLKCTRL.reg = (uint32_t)((GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK1 | (RTC_GCLK_ID << GCLK_CLKCTRL_ID_Pos)));

      // ensure module is reset
      RTC->MODE0.CTRL.bit.SWRST = 1;
      while (RTC->MODE0.CTRL.bit.SWRST == 1);

      // reset configuration is mode=0, no clear on match
      RTC->MODE0.CTRL.reg = RTC_MODE0_CTRL_PRESCALER_DIV32 | RTC_MODE0_CTRL_ENABLE;

      NVIC_EnableIRQ(RTC_IRQn);
      NVIC_SetPriority(RTC_IRQn, 0x00);

      zpmRTCSetClock(0); // reset to zero in case of warm start
}

/*******************************************************************************

    void zpmSleep(void)

    Put the chip into standby mode. The other sleep modes don't save
    enough power to be worthwhile.

******************************************************************************/
void zpmSleep(void) {
    SCB->SCR |=  SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
}


/*******************************************************************************

    void zpmPlayPossum(void)

    Use in place of zpmSleep for debugging. Spins on RTC interrupt bit.
    Cannot _WFI because the reason we are not actually sleeping is to
    let other processes continue to run that may generate interrupts,
    like USB.

******************************************************************************/
void zpmPlayPossum(void) {
    _g_f_playing_possum = true;
    while (_g_f_playing_possum);
}


/*******************************************************************************

    uint32_t zpmRTCGetClock(void)

    Return the RTC counter value.

******************************************************************************/
uint32_t zpmRTCGetClock(void) {
    return (RTC->MODE0.COUNT.reg);
}


/*******************************************************************************

    void zpmRTCSetClock(const uint32_t count)

    Set the value of the RTC counter to count. This may cause an interrupt
    to be missed.

******************************************************************************/
void zpmRTCSetClock(const uint32_t count) {
    RTC->MODE0.COUNT.reg = count;
}


/*******************************************************************************

    void zpmRTCDelay(const uint32_t count)

    Wait in a blocking loop for count ticks (approximately ms).

******************************************************************************/
void zpmRTCDelay(const uint32_t count) {
    uint32_t start = zpmRTCGetClock();
    while (zpmRTCGetClock() < (start + count));
}


/*******************************************************************************

    void zpmRTCInterruptEvery(const uint32_t count, const voidFuncPtr callback)

    Generate an interrupt every count ticks (approximately ms).

******************************************************************************/
void zpmRTCInterruptEvery(const uint32_t count, const voidFuncPtr callback) {

    zpmRTCInterruptDisable();
    _g_RTC_interrupt_interval = count;
    _g_RTC_callBack = callback;

    // clear any pending interrupts, set compare register and enable interrupt
    RTC->MODE0.INTFLAG.reg = RTC_MODE0_INTFLAG_MASK;
    RTC->MODE0.COMP[0].reg = zpmRTCGetClock() + count;
    RTC->MODE0.INTENSET.bit.CMP0 = 1;
}


/*******************************************************************************

    void zpmRTCInterruptAt(const count ms, const voidFuncPtr callback)

    Generate an interrupt when the RTC counter reaches count.

******************************************************************************/
void zpmRTCInterruptAt(const uint32_t count, const voidFuncPtr callback) {
    zpmRTCInterruptDisable();
    _g_RTC_callBack = callback;

    // clear any pending interrupts, set compare register and enable interrupt
    RTC->MODE0.INTFLAG.reg = RTC_MODE0_INTFLAG_MASK;
    RTC->MODE0.COMP[0].reg = count;
    RTC->MODE0.INTENSET.bit.CMP0 = 1;
}


/*******************************************************************************

    void zpmRTCInterruptDisable(void)

    Disable all RTC interrupts.

******************************************************************************/
void zpmRTCInterruptDisable(void) {
    _g_RTC_interrupt_interval = 0;
    RTC->MODE0.INTENCLR.reg = RTC_MODE0_INTENCLR_MASK;
}


/*******************************************************************************

    void RTC_Handler(void)

    RTC interrupt vector points here. If there are periodic interrupts,
    reset the match register. If a one-time interrupt disable interrupts
    to prevent another interrupt on timer rollover.

******************************************************************************/
void RTC_Handler(void)
{
    RTC->MODE0.INTFLAG.reg = RTC_MODE0_INTFLAG_MASK; // clear all interrupt sources

    if (_g_RTC_interrupt_interval != 0) {
        RTC->MODE0.COMP[0].reg = RTC->MODE0.COMP[0].reg + _g_RTC_interrupt_interval;
    }

    if (_g_RTC_interrupt_interval == 0) {
        zpmRTCInterruptDisable();
    }

    /*
     * Interrupts cannot be enabled without calling a function that sets
     * RTC_callback so there will never be a stale callback if interrupts
     * are enabled.
     *
     * Putting the callback at the end of the handler allows the callback
     * to set a new or different interrupt.
     */
    if (_g_RTC_callBack != NULL) { _g_RTC_callBack(); }

    _g_f_playing_possum = false;    // release fake sleep from spin loop
}





