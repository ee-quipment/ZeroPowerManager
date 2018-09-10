/*
      Demo of ZeroPowerManager. See ZeroPowerManager.jpg for a
      logarmithmic graph of current vs time for this demo.
      
      Board target: Adafruit M0 WiFi
          
      Measured power with
          LiPo charger removed, LDO removed
          Powered from a separate 3.3V supply
          All ports + spi disabled
          USB off
          
          48 MHz: 7.5 mA
           8 MHz: 1.5 mA
          32 KHz: 200 uA
          Sleep :   6 uA
*/

#include  <ZeroPowerManager.h>


void setup()
{    
    zpmRTCInit();   
    
    /*
     * To get to datasheet power levels the I/O must be disabled
     */
    zpmPortDisableDigital();
    zpmPortDisableSPI();
    zpmPortDisableUSB();
}


void loop()
{    
    /*
     * Stay in each power state for about a second.
     */
    zpmCPUClk48M();
    delay(1000);
  
    zpmCPUClk8M();    
    delay(1000/6);
    
    zpmCPUClk32K();
    delay(1);
    
    /* 
     * There is no interrupt handler (callback = NULL). Execution will
     * resume from the sleep instruction.
     */ 
    uint32_t now = zpmRTCGetClock();
    zpmRTCInterruptAt(now + 1000, NULL);
    zpmSleep();
}

