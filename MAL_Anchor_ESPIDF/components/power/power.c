/*----------------------------------------------------------------------*
 * M5Stack Bettery/Power Control Library v1.0                           *
 *                                                                      *
 * This work is licensed under the GNU Lesser General Public            *
 * License v2.1                                                         *
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html           *
 *----------------------------------------------------------------------*/
#include "Power.h"
#include "esp32/rom/rtc.h"
#include "esp_sleep.h"
//#include "esp_bt_main.h"
#include "esp_wifi.h"
#include "driver/i2c.h"
#include <stdint.h>


// ================ Power IC IP5306 ===================
#define IP5306_ADDR (117) // 0x75
#define IP5306_REG_SYS_CTL0 (0x00)
#define IP5306_REG_SYS_CTL1 (0x01)
#define IP5306_REG_SYS_CTL2 (0x02)
#define IP5306_REG_READ0 (0x70)
#define IP5306_REG_READ1 (0x71)
#define IP5306_REG_READ3 (0x78)
#define IP5306_REG_CHG_CTL0 (0x20)
#define IP5306_REG_CHG_CTL1 (0x21)
#define IP5306_REG_CHG_DIG  (0x24)

//- REG_CTL0
#define BOOST_ENABLE_BIT (0x20)
#define CHARGE_OUT_BIT (0x10)
#define BOOT_ON_LOAD_BIT (0x04)
#define BOOST_OUT_BIT (0x02)
#define BOOST_BUTTON_EN_BIT (0x01)

//- REG_CTL1
#define BOOST_SET_BIT (0x80)
#define WLED_SET_BIT (0x40)
#define SHORT_BOOST_BIT (0x20)
#define VIN_ENABLE_BIT (0x04)

//- REG_CTL2
#define SHUTDOWNTIME_MASK (0x0c)
#define SHUTDOWNTIME_64S (0x0c)
#define SHUTDOWNTIME_32S (0x04)
#define SHUTDOWNTIME_16S (0x08)
#define SHUTDOWNTIME_8S  (0x00)

//- REG_READ0
#define CHARGE_ENABLE_BIT (0x08)

//- REG_READ1
#define CHARGE_FULL_BIT (0x08)

//- REG_READ2
#define LIGHT_LOAD_BIT (0x20)
#define LOWPOWER_SHUTDOWN_BIT (0x01)

//- CHG
#define CURRENT_100MA  (0x01 << 0)
#define CURRENT_200MA  (0x01 << 1)
#define CURRENT_400MA  (0x01 << 2)
#define CURRENT_800MA  (0x01 << 3)
#define CURRENT_1600MA  (0x01 << 4)

#define BAT_4_2V      (0x00)
#define BAT_4_3V      (0x01)
#define BAT_4_3_5V    (0x02)
#define BAT_4_4V      (0x03)

#define CHG_CC_BIT    (0x20)


#define PAL_I2C_MASTER_NUM              I2C_NUM_0       /*!< I2C port number for master dev */

#define WRITE_BIT                       I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                        I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                    0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                   0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                         0x0              /*!< I2C ack value */
#define NACK_VAL                        0x1              /*!< I2C nack value */

#define LOW 0

// extern M5Stack M5;

// POWER() {
// }

//TO replace M5.I2C.readbyte in the arduino driver
_Bool Power_readByte( uint8_t i2c_Address, uint8_t registerAddress, uint8_t * data) {

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2c_Address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, registerAddress , ACK_CHECK_EN);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2c_Address << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, NACK_VAL);
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(PAL_I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return 1;
}

_Bool Power_writeByte(uint8_t i2c_Address,  uint8_t registerAddress, uint8_t data)
{
	  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i2c_Address<<1) | I2C_MASTER_WRITE , ACK_CHECK_EN);
    i2c_master_write_byte(cmd, registerAddress, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(PAL_I2C_MASTER_NUM, cmd,  pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

//    ESP_LOGI("dps310<", "TX to 0x%02X:0x%02X", reg_addr, data);

    return 1;
}


void Power_begin() {
  uint8_t data;
  

  // 450ma 
  setVinMaxCurrent(CURRENT_800MA);

  setChargeVolt(BAT_4_2V);
  
  // End charge current 200ma
  if(Power_readByte(IP5306_ADDR, 0x21, &data) == true) {
    Power_writeByte(IP5306_ADDR, 0x21, (data & 0x3f) | 0x00);
  }

  // Add volt 28mv
  if(Power_readByte(IP5306_ADDR, 0x22, &data) == true) {
    Power_writeByte(IP5306_ADDR, 0x22, (data & 0xfc) | 0x02);
  }

  // Vin charge CC
  if(Power_readByte(IP5306_ADDR, 0x23, &data) == true) {
    Power_writeByte(IP5306_ADDR, 0x23, (data & 0xdf) | 0x20);
  }
}

_Bool setPowerBoostOnOff(_Bool en) {
  uint8_t data;
  if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, &data) == true) {
    return Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, en ? (data | BOOST_SET_BIT) : (data & (~BOOST_SET_BIT)));
  }
  return false;
}

// _Bool setPowerBoostSet(_Bool en) {
//   uint8_t data;
//   if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, &data) == true) {
//     return Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, en ? (data | SHORT_BOOST_BIT) : (data & (~SHORT_BOOST_BIT)));
//   }
//   return false;
// }

// _Bool setPowerVin(_Bool en) {
//   uint8_t data;
//   if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, &data) == true) {
//     return Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, en ? (data | VIN_ENABLE_BIT) : (data & (~VIN_ENABLE_BIT)));
//   }
//   return false;
// }

// _Bool setPowerWLEDSet(_Bool en) {
//   uint8_t data;
//   if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, &data) == true) {
//       return Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, en ? (data | WLED_SET_BIT) : (data & (~WLED_SET_BIT)));
//   }
//   return false;
// }

// _Bool setPowerBtnEn(_Bool en) {
//   uint8_t data;
//   if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL0, &data) == true) {
//     return Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL0, en ? (data | BOOST_BUTTON_EN_BIT) : (data & (~BOOST_BUTTON_EN_BIT)));
//   }
//   return false;
// }

_Bool setLowPowerShutdownTime(ShutdownTime time)
{
  uint8_t data;
  _Bool ret;
  if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL2, &data) == true){
    switch (time){
      case SHUTDOWN_8S:
        ret = Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL2, ((data & (~SHUTDOWNTIME_MASK)) | SHUTDOWNTIME_8S));
        break;
      case SHUTDOWN_16S:
        ret = Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL2, ((data & (~SHUTDOWNTIME_MASK)) | SHUTDOWNTIME_16S));
        break;
      case SHUTDOWN_32S:
        ret = Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL2, ((data & (~SHUTDOWNTIME_MASK)) | SHUTDOWNTIME_32S));
        break;
      case SHUTDOWN_64S:
        ret = Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL2, ((data & (~SHUTDOWNTIME_MASK)) | SHUTDOWNTIME_64S));
        break;
      default:
        ret = false;
        break;
    }
    return ret;
  }
  return false;
}

/*
  default: false
  false: when the current is too small, ip5306 will automatically shut down
  note: it seem not work and has problems
        Function has disabled.(Stab for compatibility)
        This function will be removed in a future release.
*/
_Bool setKeepLightLoad(_Bool en)
{
  // uint8_t data;
  // if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL0, &data) == true) {
  //     return Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL0, !en ? (data | LIGHT_LOAD_BIT) : (data & (~LIGHT_LOAD_BIT)));
  // }
  return false;
}

// true: Output normally open
_Bool setPowerBoostKeepOn(_Bool en) {
  uint8_t data;
  if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL0, &data) == true) {
    return Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL0, en ? (data | BOOST_OUT_BIT)  : (data & (~BOOST_OUT_BIT)));
  }
  return false;
}

/**
* Function has disabled.(Stab for compatibility)
* This function will be removed in a future release.
*/
_Bool setLowPowerShutdown(_Bool en)
{
  //uint8_t data;
  //if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, &data) == true) {
  //  return Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, en ? (data | LOWPOWER_SHUTDOWN_BIT) : (data & (~LOWPOWER_SHUTDOWN_BIT)));
  //}
  return setPowerBoostKeepOn(!en);
}
/*
  default: true
  true: If enough load is connected, the power will turn on.
*/
// _Bool setAutoBootOnLoad(_Bool en) {
//   uint8_t data;
//   if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL0, &data) == true) {
//     return Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL0, en ? (data | BOOT_ON_LOAD_BIT) : (data & (~BOOT_ON_LOAD_BIT)));
//   }
//   return false;
// }

_Bool setVinMaxCurrent(uint8_t cur) {
  uint8_t data;
  if(Power_readByte(IP5306_ADDR, IP5306_REG_CHG_DIG, &data) == true) {
    return Power_writeByte(IP5306_ADDR, IP5306_REG_CHG_DIG, (data & 0xe0) | cur);
  }
  return false;
}

_Bool setChargeVolt(uint8_t volt) {
  uint8_t data;
  if (Power_readByte(IP5306_ADDR, IP5306_REG_CHG_CTL0, &data) == true) {
    return Power_writeByte(IP5306_ADDR, IP5306_REG_CHG_CTL0, (data & 0xfc) | volt);
  }
  return false;
}

// if charge full,try set charge enable->disable->enable,can be recharged
// _Bool setCharge(_Bool en) {
//   uint8_t data;
//   if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL0, &data) == true) {
//       return Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL0, en ? (data | CHARGE_OUT_BIT) : (data & (~CHARGE_OUT_BIT)));
//   }
//   return false;
// }

// full return true, else return false
_Bool isChargeFull() {
  uint8_t data;
  // return (Power_readByte(IP5306_ADDR, IP5306_REG_READ1, &data))
  //        ? (data & CHARGE_FULL_BIT)
  //        : false;
  if (Power_readByte(IP5306_ADDR, IP5306_REG_READ1, &data))
  {
    return data & CHARGE_FULL_BIT;
  }
  else 
  {
    return false;
  }
}

// test if ip5306 is an i2c version
// _Bool canControl() {
//   return M5.I2C.writeCommand(IP5306_ADDR, IP5306_REG_READ0);
// }

//true:charge, false:discharge
_Bool isCharging() {
  uint8_t data;;

  if (Power_readByte(IP5306_ADDR, IP5306_REG_READ0, &data))
  {
    return data & CHARGE_ENABLE_BIT;
  }
  else 
  {
    return false;
  }
}

// Return percentage * 100
int8_t getBatteryLevel() {
  uint8_t data;
  if (Power_readByte(IP5306_ADDR, IP5306_REG_READ3, &data) == true) {
    switch (data & 0xF0) {
      case 0x00:
        return 100;
      case 0x80:
        return 75;
      case 0xC0:
        return 50;
      case 0xE0:
        return 25;
      default:
        return 0;
    }
  }
  return -1;
}

void setWakeupButton(uint8_t button) {
  _wakeupPin = button;
}

void reset() {
  esp_restart();
}

_Bool isResetbySoftware() {
  RESET_REASON reset_reason = rtc_get_reset_reason(0);
  return (reset_reason == SW_RESET ||
          reset_reason == SW_CPU_RESET);
}

_Bool isResetbyWatchdog() {
  RESET_REASON reset_reason = rtc_get_reset_reason(0);
  return (reset_reason == TG0WDT_SYS_RESET ||
          reset_reason == TG1WDT_SYS_RESET ||
          reset_reason == OWDT_RESET ||
          reset_reason == RTCWDT_SYS_RESET ||
          reset_reason == RTCWDT_CPU_RESET ||
          reset_reason == RTCWDT_RTC_RESET ||
          reset_reason == TGWDT_CPU_RESET);
}

_Bool isResetbyDeepsleep() {
  RESET_REASON reset_reason = rtc_get_reset_reason(0);
  return (reset_reason == DEEPSLEEP_RESET);
}

_Bool isResetbyPowerSW() {
  RESET_REASON reset_reason = rtc_get_reset_reason(0);
  return (reset_reason == POWERON_RESET);
}

// //note:
// //If the IP5306 I2C communication is not available,
// //such as the old model, there is a limit to the maximum time for sleep return.
// //When using this function, pay attention to the constraints.
// void deepSleep(uint64_t time_in_us){

//   // Keep power keep boost on
//   setPowerBoostKeepOn(true);

//   // power off the Lcd
//   //M5.Lcd.setBrightness(0);
//   //M5.Lcd.sleep();

//   // ESP32 into deep sleep
//   esp_sleep_enable_ext0_wakeup((gpio_num_t)_wakeupPin, LOW);

//   if (time_in_us > 0){
//     esp_sleep_enable_timer_wakeup(time_in_us);
//   }else{
//     esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
//   }

//   // while (digitalRead(_wakeupPin) == LOW) {
//   //   delay(10);
//   }

//   (time_in_us == 0) ? esp_deep_sleep_start() : esp_deep_sleep(time_in_us);
// }

//note:
//If the IP5306 I2C communication is not available, 
//such as the old model, there is a limit to the maximum time for sleep return. 
//When using this function, pay attention to the constraints.
void lightSleep(uint64_t time_in_us) {

  // Keep power keep boost on
  setPowerBoostKeepOn(true);

  // power off the Lcd
  // M5.Lcd.setBrightness(0);
  // M5.Lcd.sleep();

  // ESP32 into deep sleep
  esp_sleep_enable_ext0_wakeup((gpio_num_t)_wakeupPin, LOW);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);

  // while (digitalRead(_wakeupPin) == LOW) {
  //   delay(10);
  //}
  if (time_in_us > 0){
    esp_sleep_enable_timer_wakeup(time_in_us);
  }else{
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  }
  esp_light_sleep_start();

  // power on the Lcd
  // M5.Lcd.wakeup();
  // M5.Lcd.setBrightness(200);
}

//note:
//To ensure that the power is turned off,
//reduce the power consumption according to the specifications of the power supply IC. 
//Otherwise, the power supply IC will continue to supply power.
void powerOFF(){
  uint8_t data;
  // power off the Lcd
  // M5.Lcd.setBrightness(0);
  // M5.Lcd.sleep();

  //Power off request
  if (Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, &data) == true){
    Power_writeByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, (data & (~BOOST_ENABLE_BIT)));
  }
  
  // if wifi was initialized, stop it
  wifi_mode_t mode;
  if (esp_wifi_get_mode(&mode) == ESP_OK) {
    esp_wifi_disconnect();
    esp_wifi_stop();
  }
  
  //stop bt
  //esp_bluedroid_disable();

  //disable interrupt/peripheral
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  gpio_deep_sleep_hold_dis();

  // Shutdown setting
  setPowerBoostKeepOn(false);
  setLowPowerShutdownTime(SHUTDOWN_8S);
  setPowerBtnEn(true);


  //wait shutdown from IP5306 (low-current shutdown)
  esp_deep_sleep_start();
}