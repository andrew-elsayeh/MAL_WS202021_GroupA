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



/*
 * Draws the battery indicator
 */
void draw_battery_indicator(uint8_t battery_level, bool isCharging);

/*
 * Initalizes the display and draws the welcome screen
 */
esp_err_t initialize_display(void);

#endif /* GUI_H */
