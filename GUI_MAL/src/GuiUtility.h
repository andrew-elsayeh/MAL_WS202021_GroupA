#include <stdint.h>

//GUI color settings
#define backgroundcolor WHITE
#define brightness 255
#define bordercolor BLACK


namespace GUI
{
    namespace MENU
    {
    void show_menu();

    namespace BUTTONS{
        void drawLeftButton(uint16_t, uint16_t);
        void drawBellButton(uint16_t, uint16_t);
        void drawRightButton(uint16_t, uint16_t);
        void activateRightButton(uint16_t, uint16_t);
        void activateBellButton(uint16_t, uint16_t);
        void activateLeftButton(uint16_t, uint16_t);
        void respond();
        }
    }
}