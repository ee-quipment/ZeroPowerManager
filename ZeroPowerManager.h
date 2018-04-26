/******************************************************************************

    ZeroPowerManager - Support for very low power Arduino Zero operation.

    When changing CPU speed only the output of generic clock generator 0
    (GCLK_MAIN) is changed. This affects anything running off of that generic
    clock. For basic Arduino sketches this is only the CPU clock and the SysTick
    timer, which drives the delay() counter. So delay() can still be used
    but must be scaled proportionately to the CPU clock from the default
    48 MHz.

    When the processor is put to sleep, the CPU clock will stop and SysTick
    stops and so SysTick cannot be used for timing. The RTC can be configured
    to count XOSC32K ticks to keep time through sleep events and to wake up
    the processor from sleep. The RTC is driven off XOSC32K with a divide-by-32
    prescaler resulting in approximately 1 ms ticks. More precisely, the
    timer tracks seconds in a 22.10 fractional format. The counter will
    roll over approximately every 48 days.

    Be careful setting the RTC if using RTC interrupts, the interrupt will
    get missed if the clock is set past it. zpmRTCDelay is just a spin loop,
    it's probably better to set an interrupt.

    COPYRIGHT (c) 2018 ee-quipment.com

 *****************************************************************************/

 
#ifndef _ZeroPowerManager_H_
#define _ZeroPowerManager_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef void(*voidFuncPtr)(void);


/* Port control */
void  zpmPortDisableDigital(void);
void  zpmPortDisableSPI(void);
void  zpmPortDisableUSB(void);


/* CPU clock control */
void  zpmCPUClk48M(void);     // set clock speeds. Affects sysTick()
void  zpmCPUClk8M(void);
void  zpmCPUClk32K(void);


/* Sleep control, requires RTC be configured to generate an interrupt to wake up */
void  zpmSleep(void);         // lowest power deep sleep mode.
void  zpmPlayPossum(void);    // spin loop, use in place of zpmSleep for debugging


/* Clock Functions - RTC counts seconds in a 22.10 fractional format */
void      zpmRTCInit(void);
uint32_t  zpmRTCGetClock(void);
void      zpmRTCSetClock(const uint32_t count);
void      zpmRTCDelay(const uint32_t count);
void      zpmRTCInterruptEvery(const uint32_t count, const voidFuncPtr callback);
void      zpmRTCInterruptAt(const uint32_t count, const voidFuncPtr callback);
void      zpmRTCInterruptDisable(void);


#ifdef __cplusplus
}
#endif

#endif  /* _ZeroPowerManager_H_ */


