/* CODE TO DO LIST:
> Buttons increment counters for each of the food groups, which is stored in a flash memory to ensure it can be read when rebooted after complete power loss 

> will stay in deep sleep mode for majority of time, only waking up to trigger the battery LED every 2 seconds 

> wakes up with a button press and that button is logged (for user experience) 

> Screen will also show current values with the wake-up for the battery LED

> ADC for solar and Battery, if battery below 20% activate red light, otherwise green

> Display flash memory on screen

> timer starts when waking from sleep - 10m - external timer variable likely necessary 
  All button entry goes to cache
  If clear button is pressed erase cache and reset timer
  When going to sleep store cache in flash memory and erase cache 
*/
#include <TFT_eSPI.h> // graphics and font library
#include "Button2.h"  // Buttons library

// Constants - colors and GPIO pins
#define TFT_GREY 0x5AEB // New color

#define FRUITVEG 36
#define EGGDAIRY 37
#define PROTEINS 38
#define PREPARED 25
#define MISCELLA 26
#define CLEARBTT 27

#define BTMBUTT 35
#define TOPBUTT  0

#define BATTHIGH 12
#define BATT_LOW 13

#define ADC_BATT 15
#define ADCSOLAR  2


TFT_eSPI tft = TFT_eSPI(); // Invoke library, pins defined in user_setup.h
// instantiate and intialize cache variables
int fruit = 0, egg = 0, prot = 0, prep = 0, misc = 0;
// if button pressed
bool btnClick = false;
// if initialized
bool init_ = false;

// Enable the buttons 
Button2 fv(FRUITVEG); // fruit/veg
Button2 ed(EGGDAIRY); // eggs and dairy
Button2 po(PROTEINS); // proteins (fish, chicken, meat, et cetera
Button2 pe(PREPARED); // prepared meals
Button2 ms(MISCELLA); // miscellaneous
Button2 cl(CLEARBTT); // clear (upper button)
Button2 btm(BTMBUTT); // bottom onboard button
Button2 top(TOPBUTT); // top onboard button

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void reset(int fm, int em, int pom, int pem, int mm) {
  fruit -= fm, egg -= em, prot -= pom, prep -= pem, misc -= mm;
}

void button_init()
{
  // Force long sleep by using small on-board button
    top.setLongClickHandler([](Button2 & b) {
        btnClick = false;
        int r = digitalRead(TFT_BL);
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Press again to wake up",  tft.width() / 2, tft.height() / 2 );
        espDelay(6000);
        digitalWrite(TFT_BL, !r);

        tft.writecommand(TFT_DISPOFF);
        tft.writecommand(TFT_SLPIN);
        //After using light sleep, you need to disable timer wake, because here use external IO port to wake up
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        // esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
        delay(200);
        esp_deep_sleep_start();
    });


    //Short button click event handling
    fv.setPressedHandler([](Button2 & b) {
        ++fruit;
        btnClick = true;
    });
    ed.setPressedHandler([](Button2 & b) {
        ++egg;
        btnClick = true;
    });

    po.setPressedHandler([](Button2 & b) {
        ++prot;
        btnClick = true;
    });

    pe.setPressedHandler([](Button2 & b) {
        ++prep;
        btnClick = true;
    });

    ms.setPressedHandler([](Button2 & b) {
        ++misc;
        btnClick = true;
    });
    
    cl.setPressedHandler([](Button2 & b) {
        reset(fruit, egg, prot, prep, misc);
        btnClick = true;
    });

    btm.setPressedHandler([](Button2 & b) {
        btnClick = true;
    });
}

void button_loop() 
{
  fv.loop();
  ed.loop();
  po.loop();
  pe.loop();
  ms.loop();
  cl.loop();
  btm.loop();
  top.loop();
}

void setup() {
  
  /*
  ADC_EN is the ADC detection enable port
  If the USB port is used for power supply, it is turned on by default.
  If it is powered by battery, it needs to be set to high level
  */
//  pinMode(ADC_EN, OUTPUT);
//  digitalWrite(ADC_EN, HIGH);

  tft.init();                   // Initialize display
  tft.setRotation(3);           // Set display to the correct orientation
  tft.fillScreen(TFT_GREY);     // Fill background  
  tft.setCursor(0,0,4);         // Set cursor to front, use font #4
  tft.setTextColor(TFT_WHITE);  // Set color to be white

  button_init();                // Initialize buttons
  // set cache values to -1 since they will increase by 1 upon first read
  fruit = -1, egg = -1, prot = -1, prep = -1, misc = -1;
}

void loop() {
  tft.setCursor(0,0,4); // Set cursor to first line, first column, use font #4
  // Print the variables
  tft.print(" Misc.: ");     tft.println(misc);
  tft.print(" Proteins: ");  tft.println(prot);
  tft.print(" Fruit: ");     tft.println(fruit);
  tft.print(" Prepared: ");  tft.println(prep);
  tft.print(" Egg/Dairy: "); tft.println(egg);

  // Button loop
  if (btnClick) {
    btnClick = false; // reset trigger state after it was pressed
    tft.fillScreen(TFT_GREY); // Clear the screen 
  }
  button_loop(); // Loop through and poll the buttons.
}
