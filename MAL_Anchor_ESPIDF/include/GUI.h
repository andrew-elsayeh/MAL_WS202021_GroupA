#ifndef GUI_H
#define GUI_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"

#include "power.h"
#include "tftspi.h"
#include "tft.h"


#define BATTERY_INDICATOR_COORDINATE_X 285
#define BATTERY_INDICATOR_COORDINATE_Y 70

#define SPI_BUS TFT_HSPI_HOST   // Define which spi bus to use TFT_VSPI_HOST or TFT_HSPI_HOST



extern const color_t TFT_IFX_CYAN;
extern const color_t TFT_IFX_GREEN;
extern const color_t TFT_IFX_MAGENTA;
extern const color_t TFT_IFX_GREY;

extern char* menu;
extern char* help_text;

/*
 * Booleans to define the state-machine
 */
extern bool publish_active;    //Publish can only be actibe if error, qr_code and help are not active
extern bool qr_code_active;    //Can only be active if error and help are not active
extern bool error_active;      //Activated by any error, disables all buttons
extern bool help_active;       //Can only be active if error and qr_code are not active

/*
 * Draws the battery indicator
 */
void draw_battery_indicator(uint8_t battery_level, bool isCharging);

/** Displays the Footer / Menu on the bottom of the screen
* @param info: String that gets printed in the footer
*/
void disp_footer(char *info);

/** Displays the default screen
 * Loads a background picture from spiffs memory, shows the boxes for DPS310 and Trust M, prints info about the modem
 * @param module_info: Info about the modem and SIM. ex: IMEI, ICCID..
 * @param network_info: Info about the internet connection. ex: IP Address
 * @param dps310_active: Flag to keep track if the DPS is connected or not
 * @param trustm_active: Flag to keep track if the Trust M is connected or not
 */
void show_default(char *module_info, char *network_info, bool dps310_active, bool trustm_active);

/** Shows the help screen
 * Help message (help_text) is a globally defined variable
 */
void show_help();

/** Shows error message in the footer of the screen and highligting it
 * @param error: Error message displayed in the footer
 */
void show_error(char *error);


/** Shows info in the default screen, only possible if neither qr_code nor help are active
 * @param info: Content printed in the main screen, usually info about the modem
 */
void show_info(char *info);

/** Shows qr_code **/
void show_qr_code();

/*
 * Initalizes the display and draws the welcome screen
 */
void initialize_display(void);

#endif /* GUI_H */
