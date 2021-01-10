/*----------------------------------------------------------------------*
 * M5Stack Bettery/Power Control Library v1.0                           *
 *                                                                      *
 * This work is licensed under the GNU Lesser General Public            *
 * License v2.1                                                         *
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html           *
 *----------------------------------------------------------------------*/
#include <stdint.h>

#ifndef Power_h
  #define Power_h

  #define SLEEP_MSEC(us) (((uint64_t)us) * 1000L)
  #define SLEEP_SEC(us) (((uint64_t)us) * 1000000L)
  #define SLEEP_MIN(us) (((uint64_t)us) * 60L * 1000000L)
  #define SLEEP_HR(us) (((uint64_t)us) * 60L * 60L * 1000000L)

 // class POWER
  //{
    //public:
      // POWER();
      // _Bool canControl();
      void Power_begin();

      // -- ShutdownTimeParam
      typedef enum 
      {
        SHUTDOWN_8S = 0,
        SHUTDOWN_16S,
        SHUTDOWN_32S,
        SHUTDOWN_64S
      } ShutdownTime;

      _Bool Power_readByte( uint8_t i2c_Address, uint8_t registerAddress, uint8_t * data);
      _Bool Power_writeByte(uint8_t i2c_Address,  uint8_t registerAddress, uint8_t data);

      // -- control for power
      _Bool setKeepLightLoad(_Bool en) __attribute__((deprecated));
      _Bool setPowerBoostKeepOn(_Bool en);
      _Bool setAutoBootOnLoad(_Bool en);
      _Bool setLowPowerShutdown(_Bool en) __attribute__((deprecated));    
      _Bool setLowPowerShutdownTime(ShutdownTime time);
      _Bool setPowerBoostOnOff(_Bool en);
      _Bool setPowerBoostSet(_Bool en);
      _Bool setPowerVin(_Bool en);
      _Bool setPowerWLEDSet(_Bool en);
      _Bool setPowerBtnEn(_Bool en);

      // -- control for battery
      _Bool setVinMaxCurrent(uint8_t cur);
      _Bool setChargeVolt(uint8_t volt);

      _Bool setCharge(_Bool en);
      _Bool isChargeFull();
      _Bool isCharging();
      int8_t getBatteryLevel();
      _Bool batteryMode(_Bool en);

      // -- configuration for wakeup
      void setWakeupButton(uint8_t button);

      // -- get resson for startup
      _Bool isResetbyWatchdog();
      _Bool isResetbyDeepsleep();
      _Bool isResetbySoftware();
      _Bool isResetbyPowerSW();

      // -- sleep
      void deepSleep(uint64_t time_in_us);
      void lightSleep(uint64_t time_in_us);

      // -- power off
      void powerOFF();

      // -- software reset
      void reset();

    //private:
      uint8_t _wakeupPin;
#endif
