/**
 * @file main.c
 * @brief Firmware for BLE Anchor using to ping various tags 
 * @author Andrew Elsayeh
 * 
 * This program utilizies BLE to connect a specific BLE Server based on user input, and enables 
 * executing specific commands such as pinging the device which can be used to localize objects 
 * based on sound or light
 */
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

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/i2c.h"


#include "esp_spiffs.h"

#include "button.h"
#include "power.h"
#include "GUI.h"



// =============================================================================
// Tasks
// =============================================================================
TaskHandle_t ButtonHandler = NULL;
TaskHandle_t GUI_Task = NULL;
TaskHandle_t GUI_RingingAnimation = NULL;

// =============================================================================
// Semaphores
// =============================================================================
/*
 * Since the screen is a shared resource, this mutex is used to protect 
 * against two tasks trying to write to the screen at the same time and causing
 * a crash
*/
SemaphoreHandle_t ScreenLock = NULL;

// =============================================================================
// Queues
// =============================================================================
QueueHandle_t button_events = NULL;
QueueHandle_t GUI_Queue = NULL;



// =============================================================================
// Definitions and Macros
// =============================================================================
#define BUTTON_A 39
#define BUTTON_B 38
#define BUTTON_C 37
#define BUTTON_INPUT_PIN_SEL ( (1ULL<<BUTTON_A) | (1ULL<<BUTTON_B) | (1ULL<<BUTTON_C) )
#define ESP_INTR_FLAG_DEFAULT 0

#define PAL_I2C_MASTER_SCL_IO           	22                          /*!< gpio number for I2C master clock */
#define PAL_I2C_MASTER_SDA_IO          		21                          /*!< gpio number for I2C master data */
#define PAL_I2C_MASTER_FREQ_HZ              400000
#define I2C_MASTER_TX_BUF_DISABLE           0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE           0                           /*!< I2C  master doesn't need buffer */
#define PAL_I2C_MASTER_NUM                  I2C_NUM_0

#define GUI_QUEUE_LENGTH 5

#define STOP_RINGING 0x01
#define START_RINGING 0x02

// =============================================================================
// Global Variables
// =============================================================================
button_event_t ButtonEvent; //Used for handling button presses

static const char *TAG = "MAL BLE Anchor";

// =============================================================================
// Structures and Enums
// =============================================================================
typedef enum{
    COUNT_UP = 0,       /*!< Increment counter that points to device being displayed 
                            on the screen */
    COUNT_DOWN = 1,     /*!< Decrement counter that points to device being displayed 
                            on the screen */
    TOGGLE_RINGING = 2, /*!< Toggle beeping the device being looked for */ 
} GUI_action_t;

typedef enum{
    KEYS = 0,
    WALLET = 1,
    PILL_BOX = 2,
}  Objects_t;



/*
 * This task is responsible for handling button presses. It is blocked until a button press
 * is detected. The debouncing of the buttons is proceced in the background, and this task
 * only receives a button press when it is valid
 */
static void vButtonHandler(void* arg)
{
    //const char *BTN_TAG = "Buttons";
    GUI_action_t GUI_Action;
    bool RingingState = false;

    while (true) {
        if (xQueueReceive(button_events, &ButtonEvent, 1000/portTICK_PERIOD_MS) == pdTRUE) {
            if (ButtonEvent.event == BUTTON_DOWN){

                if ((ButtonEvent.pin == BUTTON_A) && (ButtonEvent.event == BUTTON_DOWN)) {
                    GUI_Action = COUNT_DOWN;
                    xQueueSend(GUI_Queue, &GUI_Action, 0);
                }
                if ((ButtonEvent.pin == BUTTON_B) && (ButtonEvent.event == BUTTON_DOWN)) {
                    RingingState = !RingingState;           //Toggle Ringing State
                    GUI_Action = TOGGLE_RINGING;    
                    xQueueSend(GUI_Queue, &GUI_Action, 0);
                }
                if ((ButtonEvent.pin == BUTTON_C) && (ButtonEvent.event == BUTTON_DOWN)) {
                    GUI_Action = COUNT_UP;
                    xQueueSend(GUI_Queue, &GUI_Action, 0);
                }
            }
        }
    }
}

#define ANIMATION_DELAY 150
void draw_ringing_animation(){

    if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE){
        TFT_jpg_image(15,75,0, "/spiffs/soundwave_left_1.jpg", NULL, 0);
        TFT_jpg_image(tft_width - 55,75,0, "/spiffs/soundwave_right_1.jpg", NULL, 0);
        xSemaphoreGive(ScreenLock);
    }
    vTaskDelay(pdMS_TO_TICKS(ANIMATION_DELAY));
    if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE){
        TFT_jpg_image(15,75,0, "/spiffs/soundwave_left_2.jpg", NULL, 0);
        TFT_jpg_image(tft_width - 55,75,0, "/spiffs/soundwave_right_2.jpg", NULL, 0);
        xSemaphoreGive(ScreenLock);
    }
    vTaskDelay(pdMS_TO_TICKS(ANIMATION_DELAY*2));
    if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE){
        TFT_fillRect(15,75,40,100,TFT_WHITE);
        TFT_fillRect(tft_width - 55,75,40,100,TFT_WHITE);
        xSemaphoreGive(ScreenLock);
    }
    vTaskDelay(pdMS_TO_TICKS(ANIMATION_DELAY));
}

static void vGUI_RingingAnimation(void* arg){
    uint32_t RecievedNotification; //Buffer to hold task notification with which different drawing flags are set
    bool Ringing = false;
    while (1)
    {
        RecievedNotification = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));    //Check if still supposed to ring

        switch (RecievedNotification)
        {
        case START_RINGING:
            Ringing = true;
            break;
        case STOP_RINGING:
            Ringing = false;    
        default:
            break;
        }
        if (Ringing){
            draw_ringing_animation();
        }


    }
    
}

static void vGUI_task(void* arg){
    Objects_t Object = KEYS;            //Used to keep track of which device is being shown on the screen
    GUI_action_t GUI_Action;    
    bool RingingState = false;  //Used to keep track of whether the device is ringing or not
    
    while (1)
    {
        if (xQueueReceive(GUI_Queue, &GUI_Action, portMAX_DELAY) == pdTRUE) {
            
            switch (GUI_Action){
                case (COUNT_DOWN):
                    Object--;
                    break;
                case (COUNT_UP):
                    Object++;
                    break;
                case (TOGGLE_RINGING):
                    RingingState = !RingingState;           //Toggle Ringing State
                    if(RingingState == true){
                        xTaskNotify(GUI_RingingAnimation, START_RINGING, eSetValueWithOverwrite);
                    }
                    else{
                        xTaskNotify(GUI_RingingAnimation, STOP_RINGING, eSetValueWithOverwrite);
                    }  
                    break;
                default:
                    break;
            }

            if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE){
                switch (Object%3){
                    case (KEYS):
                        TFT_jpg_image(0,0,0, "/spiffs/keys.jpg", NULL, 0);
                        break;
                    case (WALLET):
                        TFT_jpg_image(0,0,0, "/spiffs/wallet.jpg", NULL, 0);
                        break;
                    case (PILL_BOX):
                        TFT_jpg_image(0,0,0, "/spiffs/pillbox.jpg", NULL, 0);
                        break;
                    default:
                        break;
                }
                xSemaphoreGive(ScreenLock);
            }
        }
    }
}

/*
 * This functions initializes the SPIFFS. The files in the data folder are loaded onto SPIFFS
 * and can be accessed after the initialization
 * 
 * Returns ESP_OK when succesful
 */
esp_err_t initialize_spiffs(){
    /**** Initialize SPIFFS File System ****/
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
    };
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ESP_OK;
}


static esp_err_t i2c_master_init()
{
    int i2c_master_port = PAL_I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = PAL_I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = PAL_I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = PAL_I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE, 0);
}

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


void initialize_power_management(void)
{
    Power_begin();
    
    //setPowerBoostKeepOn(1);

    uint8_t data;

    Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL0, &data);
    printf("CTL0: %d\n\r",data);

    Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL1, &data);
    printf("CTL1: %d\n\r",data);
    
    Power_readByte(IP5306_ADDR, IP5306_REG_SYS_CTL2, &data);
    printf("CTL2: %d\n\r",data);
}

void app_main() {
    initialize_power_management();

    ESP_ERROR_CHECK( initialize_spiffs() );
    
    ESP_ERROR_CHECK( i2c_master_init() );

    /**** Initialize Display and SPI-Driver and displays the welcome screen ****/
    initialize_display();


    /* 
     * Initializes Buttons and returns a handle to the queue where debounced 
     * button events are sent
     */
    button_events = button_init(PIN_BIT(BUTTON_A) | PIN_BIT(BUTTON_B) | PIN_BIT(BUTTON_C));

    ScreenLock = xSemaphoreCreateMutex();
    if (!ScreenLock){
        ESP_LOGE(TAG, "Failed to create screen lock");
    }

    GUI_Queue = xQueueCreate(GUI_QUEUE_LENGTH, sizeof(GUI_action_t));
    if (!GUI_Queue){
        ESP_LOGE(TAG, "Failed to create screen lock");
    }
    
    xTaskCreate(vButtonHandler, "Button Handler", 2048, NULL, configMAX_PRIORITIES - 1 , &ButtonHandler);
    xTaskCreate(vGUI_task, "GUI Task", 2048, NULL, configMAX_PRIORITIES - 2, &GUI_Task);
    xTaskCreate(vGUI_RingingAnimation,"Ring Animation", 3000, NULL, configMAX_PRIORITIES - 3, &GUI_RingingAnimation);

}