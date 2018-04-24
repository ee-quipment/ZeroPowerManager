### ZeroPowerManager
Library to support very low power Arduino Zero operation.

### Description
Supports the Arduino Zero SAMD21G18. Allows you to achieve 5 uA sleep current. Tested on the Feather M0 board family. Uses the 32KHz external oscillator and RTC to wake up from sleep. The timer granularity is 1 mS.

### Installation
Copy the ZeroPowerManager.h and ZeroPowerManager.c files to a ZeroPowerManager folder in your libraries directory. The library is C code with a C++ guard so it will compile cleanly in your Arduino sketch.

### Useage
Achieving data-sheet level sleep currents takes more than putting the processor in sleep mode. All internal modules must be turned off and unterminated inputs must be disabled.

On reset, the Arduino framework configures all I/Os as inputs and turns on the USB port. Unterminated inputs will oscillate and consume substantial current and so they must be disabled. The USB port must be disabled as well. It will disconnect anyway when the processor is put to sleep.

Although current consumption is reduced when running the CPU at lower clock speeds, the reduction in current is not linear. The most effective power management method is to run at full speed when doing work and sleep as much as possible. Reducing the clock speed is only effective if you have to keep things running while waiting for something where you can't sleep the processor, like receiving a radio message.

The processor idle modes are not supported in this library because they don't reduce current consumption enough to really make a difference.

When debugging, you won't want to put the processor to sleep because you'll lose the USB connection. Instead, you'll want to [play possum](http://www.dictionary.com/browse/play--possum). This will simulate going to sleep the processor will continue to run and will maintain the USB connection. A #define can make this easy:

    #define zpmSleep  zpmPlayPossum
  
For lowest power consumption all I/Os must be disabled. This may mean having to reinitialize SPI peripherals and I/O ports after waking up.

    zpmPortDisableUSB();
    zpmPortDisableSPI();
    zpmPortDisableDigital();
    zpmSleep();
    
Don't forget to configure an interrupt, either the RTC or an external pin, or else the processor will never wake up.

When you (inevitably) find that you are not seeing the current consumption that you expect, that there are other parts that are also drawing current. On the Feather M0 boards the LDO regulator and the LiPo charger will draw about 50 uA each, so right there you are at least an order of magnitude over what is possible to achieve.

### API
#### Port Control
**void  zpmPortDisableDigital(void)**  
Disables all of the digital I/O by returning them to their power-up configuration where they are neither inputs nor outputs and prevents them from oscillating and drawing current.

**void  zpmPortDisableSPI(void)**  
Disables the SPI port pins by returning them to their power-up configuration where they are neither inputs nor outputs and prevents them from oscillating and drawing current.

**void  zpmPortDisableUSB(void)**  
Completely turns off the USB module.

#### CPU Clock Control

Reducing the clock speed will reduce current consumption but will also affect the SysTick counter which is used as a reference for the Arduino millisec() and delay() functions. You can still use these timing functions but you must scale them by the CPU clock frequency. For example, at a CPU clock frequency of 8 MHz, delay(1) will delay for 6 ms.

**void  zpmCPUClk48M(void)**

**void  zpmCPUClk8M(void)**

**void  zpmCPUClk32K(void)**

#### Sleep Control
**void  zpmSleep(void)**

**void  zpmPlayPossum(void)**

#### RTC Control
**void  zpmRTCInit(void)**

**uint32_t  zpmRTCGetClock(void)**

**void  zpmRTCSetClock(const uint32_t count)**

**void  zpmRTCDelay(const uint32_t count)**

**void  zpmRTCInterruptEvery(const uint32_t count, const voidFuncPtr callback)**

**void  zpmRTCInterruptAt(const uint32_t count, const voidFuncPtr callback)**

**void  zpmRTCInterruptDisable(void)**







### License
See the LICENSE file for license rights and limitations (MIT).
