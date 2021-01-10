#include "GUI.h"

//Used for logging
static const char *TAG = "GUI";


void draw_battery_indicator(uint8_t battery_level, bool isCharging)
{
    TFT_saveClipWin();
    TFT_resetclipwin();
    if (isCharging){
        switch (battery_level)
        {
            case 100:
                TFT_jpg_image(BATTERY_INDICATOR_COORDINATE_X,BATTERY_INDICATOR_COORDINATE_Y,0, "/spiffs/charging_4.jpg", NULL, 0);
                break;
            case 75:
                TFT_jpg_image(BATTERY_INDICATOR_COORDINATE_X,BATTERY_INDICATOR_COORDINATE_Y,0, "/spiffs/charging_3.jpg", NULL, 0);
                break;
            case 50:
                TFT_jpg_image(BATTERY_INDICATOR_COORDINATE_X,BATTERY_INDICATOR_COORDINATE_Y,0, "/spiffs/charging_2.jpg", NULL, 0);
                break;
            case 25:
                TFT_jpg_image(BATTERY_INDICATOR_COORDINATE_X,BATTERY_INDICATOR_COORDINATE_Y,0, "/spiffs/charging_1.jpg", NULL, 0);
                break;
            case 00:
                TFT_jpg_image(BATTERY_INDICATOR_COORDINATE_X,BATTERY_INDICATOR_COORDINATE_Y,0, "/spiffs/charging_0.jpg", NULL, 0);
                break;

        }
    } else
    {
        switch (battery_level)
        {
            case 100:
                TFT_jpg_image(BATTERY_INDICATOR_COORDINATE_X,BATTERY_INDICATOR_COORDINATE_Y,0, "/spiffs/battery_4.jpg", NULL, 0);
                break;
            case 75:
                TFT_jpg_image(BATTERY_INDICATOR_COORDINATE_X,BATTERY_INDICATOR_COORDINATE_Y,0, "/spiffs/battery_3.jpg", NULL, 0);
                break;
            case 50:
                TFT_jpg_image(BATTERY_INDICATOR_COORDINATE_X,BATTERY_INDICATOR_COORDINATE_Y,0, "/spiffs/battery_2.jpg", NULL, 0);
                break;
            case 25:
                TFT_jpg_image(BATTERY_INDICATOR_COORDINATE_X,BATTERY_INDICATOR_COORDINATE_Y,0, "/spiffs/battery_1.jpg", NULL, 0);
                break;
            case 00:
                TFT_jpg_image(BATTERY_INDICATOR_COORDINATE_X,BATTERY_INDICATOR_COORDINATE_Y,0, "/spiffs/battery_0.jpg", NULL, 0);
                break;
        }
    }
    TFT_restoreClipWin();
    

}


void initialize_display(void)
{
    esp_err_t err;


    tft_max_rdclock = 8000000;
    // Pins MUST be initialized before SPI interface initialization 
    TFT_PinsInit();

    // Configure SPI Devices

    spi_lobo_device_handle_t spi;
	
    spi_lobo_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,				// set SPI MISO pin
        .mosi_io_num=PIN_NUM_MOSI,				// set SPI MOSI pin
        .sclk_io_num=PIN_NUM_CLK,				// set SPI CLK pin
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
		.max_transfer_sz = 6*1024,
    };
    spi_lobo_device_interface_config_t devcfg={
        .clock_speed_hz=8000000,                // Initial clock out at 8 MHz
        .mode=0,                                // SPI mode 0
        .spics_io_num=-1,                       // we will use external CS pin
		.spics_ext_io_num=PIN_NUM_CS,           // external CS pin
		.flags=LB_SPI_DEVICE_HALFDUPLEX,        // ALWAYS SET  to HALF DUPLEX MODE!! for display spi
    };
    
    vTaskDelay(500 / portTICK_RATE_MS);
    printf("Pins used: miso=%d, mosi=%d, sck=%d, cs=%d\r\n", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
    
    // Initialize the SPI bus and attach the LCD to the SPI bus 
	err=spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
    assert(err==ESP_OK);
	printf("SPI: display device added to spi bus (%d)\r\n", SPI_BUS);
	tft_disp_spi = spi;
    // Test select/deselect
	err = spi_lobo_device_select(tft_disp_spi, 1);
    assert(err==ESP_OK);
	err = spi_lobo_device_deselect(tft_disp_spi);
    assert(err==ESP_OK);

	printf("SPI: attached display device, speed=%u\r\n", spi_lobo_get_speed(spi));
	printf("SPI: bus uses native pins: %s\r\n", spi_lobo_uses_native_pins(spi) ? "true" : "false");
    
    // Initialize the Display
	printf("SPI: display init...\r\n");
	TFT_display_init();
#ifdef TFT_START_COLORS_INVERTED
	TFT_invertDisplay(1);
#endif

	// Detect maximum read speed
	tft_max_rdclock = find_rd_speed();
	printf("SPI: Max rd speed = %u\r\n", tft_max_rdclock);

    // Set SPI clock used for display operations
	spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);
	printf("SPI: Changed speed to %u\r\n", spi_lobo_get_speed(spi));

	tft_font_rotate = 0;
	tft_text_wrap = 0;
	tft_font_transparent = 0;
	tft_font_forceFixed = 0;
	tft_gray_scale = 0;
    TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
	TFT_setRotation(LANDSCAPE);
	TFT_setFont(DEFAULT_FONT, NULL);
	TFT_resetclipwin();

    ESP_LOGI(TAG, "Reading Welcome File");

    // test if filesystem works correctly
    FILE* f = fopen("/spiffs/welcome.jpg", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open welcome.jpg");
        return;
    }
    fclose(f);
    
    TFT_jpg_image(0,0,0, "/spiffs/welcome.jpg", NULL, 0);


    vTaskDelay(100/portTICK_RATE_MS);
    tft_fg = TFT_BLACK;
    tft_bg = TFT_WHITE;

}