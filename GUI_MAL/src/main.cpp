#include <Arduino.h>
#include <M5Stack.h>



//GUI-settings
#include "GuiUtility.h"


void setup() {
  // put your setup code here, to run once:
  M5.begin();

  M5.Lcd.setBrightness(brightness);
  M5.Lcd.fillScreen(backgroundcolor);

}


void loop() {
  // put your main code here, to run repeatedly:
 GUI::MENU::show_menu();
 GUI::MENU::BUTTONS::respond();
}



