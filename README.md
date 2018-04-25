### ZeroPowerManager
Library to support very low power Arduino Zero operation.

### Description
Supports the Arduino Zero SAMD21G18. Allows you to achieve 6 uA sleep current. Tested on the Feather M0 board family. Uses the 32KHz external oscillator and RTC to wake up from sleep. The timer granularity is 1 mS.

### Results
Here are some measured results to give you an idea of where your current consumption should be once everthing has been configured correctly. Remember to add back in any other current draws in your system like regulators and sensors.

   Measured Adafruit Feather M0 WiFi with:
   - LiPo removed, LDO removed
   - Powered from separate 3.3V supply
   - All ports + spi disabled
   - USB off
          
   48 MHz: 7.5 mA  
   8 MHz: 1.5 mA  
   32 KHz: 200 uA  
   Sleep :   6 uA  


### Installation
Copy the ZeroPowerManager.h and ZeroPowerManager.c files to a ZeroPowerManager folder in your libraries directory. The library is C code with a C++ guard so it will compile cleanly in your Arduino sketch.

### Useage
Achieving data-sheet level sleep currents takes more than putting the processor in sleep mode. All internal modules must be turned off and unterminated inputs must be disabled.

On reset, the Arduino framework configures all I/Os as inputs and turns on the USB port. Unterminated inputs will oscillate and consume substantial current and so they must be disabled. The USB port must be disabled as well. It will disconnect anyway when the processor is put to sleep.

Although current consumption is reduced when running the CPU at lower clock speeds, the reduction in current is not linear. The most effective power management method is to run at full speed when doing work and sleep as much as possible. Reducing the clock speed is only effective if you have to keep things running while waiting for something where you can't sleep the processor, like receiving a radio message.

The processor idle modes are not supported in this library because they don't reduce current consumption enough to really make a difference.

When debugging, you won't want to put the processor to sleep because you'll lose the USB connection. Instead, you'll want to [play possum](http://www.dictionary.com/browse/play--possum). This will simulate going to sleep. The processor will continue to run and will maintain the USB connection. A #define can make this easy:

    #define zpmSleep  zpmPlayPossum     // while debugging
  
For lowest power consumption all I/Os must be disabled. This may mean having to reinitialize SPI peripherals and I/O ports after waking up. Depending upon what you are connected to the sleep sequence should look something like:

    zpmPortDisableUSB();
    zpmPortDisableSPI();
    zpmPortDisableDigital();
    zpmSleep();
    
Don't forget to configure an interrupt before calling *zpmSleep()*, either the RTC or an external pin, or else the processor will never wake up.

When you (inevitably) find that you are not seeing the current consumption that you expect remember that there are other parts that are also drawing current. On the Feather M0 boards the LDO regulator and the LiPo charger will draw about 50 uA each, so right there you are at least an order of magnitude over what is possible to achieve.

### API
#### Port Control
**void  zpmPortDisableDigital(void)**  
Disables all of the digital I/Os by returning them to their power-up configuration where they are neither inputs nor outputs and prevents them from oscillating and drawing current.

**void  zpmPortDisableSPI(void)**  
Disables the SPI port pins by returning them to their power-up configuration where they are neither inputs nor outputs and prevents them from oscillating or sinking or sourcing current to a peripheral device.

**void  zpmPortDisableUSB(void)**  
Completely turns off the USB module.

#### CPU Clock Control

Reducing the clock speed will reduce current consumption but will also affect the SysTick counter which is used as a reference for the Arduino millisec() and delay() functions. You can still use these timing functions but you must scale them by the CPU clock frequency. For example, at a CPU clock frequency of 8 MHz, delay(1) will delay for 6 ms.

**void  zpmCPUClk48M(void)**  
Set the processor clock to its maximum frequency of 48 MHz.

**void  zpmCPUClk8M(void)**  
Set the processor clock frequency to 8 MHz.

**void  zpmCPUClk32K(void)**  
Set the processor clock frequency to 32 KHz. Be careful, this is &nbsp;&nbsp;&nbsp;s&nbsp;&nbsp;&nbsp;l&nbsp;&nbsp;&nbsp;o&nbsp;&nbsp;&nbsp;w.

#### Sleep Control

**void  zpmSleep(void)**  
Put the processor into deep sleep mode. This is the lowest possible power state.

**void  zpmPlayPossum(void)**  
Execute in a spin loop until there is an interrupt. This will consume about 10 mA of current at 48 MHz, but is useful for debugging.

#### RTC Control

The Real-Time-Clock is configured as a 32 bit counter clocked by the 32.768 KHz external oscillator with a divide-by-32 prescaler. The counter represents seconds in a 22.10 fixed-point [Q format](https://en.wikipedia.org/wiki/Q_(number_format)). One counter tick is 0.977 ms, so it can be considered to be a ms as long as 2% accuracy is acceptable. The counter will roll over approximately every 48 days.

Setting the clock must be done carefully if an interrupt has been set. If the clock is set to a count greater than the interrupt count then the interrupt will not occur until the counter rolls over.

Only one interrupt can be active at any time. Calling *zpmRTCInterruptEvery()* or *zpmRTCInterruptAt()* will overwrite any previous settings.

**void  zpmRTCInit(void)**  
This must be called before the RTC can be used. If it is not called the other RTC functions will fail silently.

**uint32_t  zpmRTCGetClock(void)**  
Return the value of the 32 bit RTC counter.

**void  zpmRTCSetClock(const uint32_t count)**  
Set the value of the 32 bit RTC counter to *count*.

**void  zpmRTCDelay(const uint32_t count)**  
Execute in a spin loop for approximately *count* ms.

**void  zpmRTCInterruptEvery(const uint32_t count, const voidFuncPtr callback)**  
Generate an interrupt and execute the *callback()* function every *count* ms. *callback* may be NULL.

**void  zpmRTCInterruptAt(const uint32_t count, const voidFuncPtr callback)**  
When the RTC counter reaches *count* execute the *callback()* function. *callback* may be NULL.

**void  zpmRTCInterruptDisable(void)**  
Disable RTC interrupts. To re-enable interrupts, call either *zpmRTCInterruptEvery()* or *zpmRTCInterruptAt()*.







### License
See the LICENSE file for license rights and limitations (MIT).
