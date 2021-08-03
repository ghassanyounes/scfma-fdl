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
#include <Preferences.h> // Data Logging
#include <string>
using std::string;

//#define DEEP_SLEEP

// 10 minutes: 600000000
//  7 minutes: 420000000
//  5 minutes: 300000000
#define TIME_AWAKE 5000

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

// the active pins (38,37,36 et cetera) to wake up the device
#define PINS 1 << FRUITVEG | 1 << EGGDAIRY | 1 << PROTEINS | 1 << PREPARED | 1 << MISCELLA | 1 << CLEARBTT

#define ADC_EN  14  //ADC_EN is the ADC detection enable port
#define ADC_BAT 33
#define ADC_SOL 32

bool clear1 = false, clear2 = false, clear3 = false;

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in user_setup.h
int counter = 0;            // Counter for time to sleep
int fruit = 0, egg = 0, prot = 0, prep = 0, misc = 0;           // instantiate and intialize cache variables
int f_fruit = 0, f_egg = 0, f_prot = 0, f_prep = 0, f_misc = 0; // instantiate and initialize flash storage
bool btnClick = false; // if a button is pressed
int init_ = 0; // if initialized check
double battery_level = 0.0; // checks the battery level
double solar_level = 0.0;   // checks the solar panel power level

Preferences prefs; // Data to be preserved after reboots

// Enable the buttons 
Button2 b_fruitveg(FRUITVEG); // fruit/veg
Button2 b_eggdairy(EGGDAIRY); // eggs and dairy
Button2 b_proteins(PROTEINS); // proteins (fish, chicken, meat, et cetera
Button2 b_prepared(PREPARED); // prepared meals
Button2 b_miscella(MISCELLA); // miscellaneous
Button2 b_clear(CLEARBTT); // clear (upper button)
Button2 btm(BTMBUTT); // bottom onboard button
Button2 top(TOPBUTT); // top onboard button

//! For longer delays it's best to use light sleep, especially to reduce current consumption
void espDelay(int b_miscella)
{
    esp_sleep_enable_timer_wakeup(b_miscella * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

// When clearing or undoing 
void reset(int fm, int em, int pom, int pem, int mm) {
  fruit -= fm, egg -= em, prot -= pom, prep -= pem, misc -= mm;
}

void hardreset() {
  tft.fillScreen(TFT_GREY);     // Clear the screen
  tft.setCursor(25,50,4);         // Set cursor to front, use font #4
  tft.setTextColor(TFT_RED);    // Set color to be red
  tft.println("!! TOTAL RESET !!");
  delay(5000);
  tft.fillScreen(TFT_GREY);     // Clear the screen
  tft.setTextColor(TFT_WHITE);  // Set color to be red
  prefs.putUInt("fruit", 0);
  prefs.putUInt("egg"  , 0);
  prefs.putUInt("prot" , 0);
  prefs.putUInt("prep" , 0);
  prefs.putUInt("misc" , 0);
  fruit = 0;
  egg   = 0;
  prot  = 0;
  prep  = 0;
  misc  = 0;
}

// Button press handlers and routines
void button_init()
{
  tft.fillScreen(TFT_GREY); // Clear the screen 
  btnClick = true;

  //Short button click event handling
  b_fruitveg.setPressedHandler([](Button2 & b) {
    btnClick = true;
    tft.fillScreen(TFT_GREY); // Clear the screen
    ++fruit;
  });
  b_eggdairy.setPressedHandler([](Button2 & b) {
    btnClick = true;
    tft.fillScreen(TFT_GREY); // Clear the screen
    ++egg;
  });

  b_proteins.setPressedHandler([](Button2 & b) {
    btnClick = true;
    tft.fillScreen(TFT_GREY); // Clear the screen
    ++prot;
  });

  b_prepared.setPressedHandler([](Button2 & b) {
    btnClick = true;
    tft.fillScreen(TFT_GREY); // Clear the screen
    ++prep;
  });

  b_miscella.setPressedHandler([](Button2 & b) {
    btnClick = true;
    tft.fillScreen(TFT_GREY); // Clear the screen
    ++misc;
  });
  
  b_clear.setPressedHandler([](Button2 & b) {
    btnClick = true;
    tft.fillScreen(TFT_GREY); // Clear the screen
    reset(fruit, egg, prot, prep, misc);
  });

  btm.setPressedHandler([](Button2 & b) {
    tft.fillScreen(TFT_GREY);     // Clear the screen
    tft.setCursor(0,0,4);         // Set cursor to front, use font #4
    tft.setTextColor(TFT_RED);    // Set color to be red
    tft.print("Battery Status "); tft.println(battery_level);
    tft.print("Solar Status "); tft.println(solar_level);
    delay(5000);
    tft.fillScreen(TFT_GREY);     // Clear the screen
    tft.setTextColor(TFT_WHITE);  // Set color to be white
    btnClick = true;
  });

  // Erase everything while long clicking the clear button
  top.setLongClickHandler([](Button2 & b) {
    btnClick = true;
    hardreset();
  });


  // Hard reset: press these 3 buttons at the same time
  b_eggdairy.setLongClickHandler([](Button2 & b) {
    clear1 = true;
  });
  b_prepared.setLongClickHandler([](Button2 & b) {
    clear2 = true;
  });
  b_clear.setLongClickHandler([](Button2 & b) {
    clear3 = true;
  });

}

// Store the variables in the EEPROM flash storage
void store() { 
  f_fruit += fruit;
  f_egg   += egg;
  f_prot  += prot;
  f_prep  += prep;
  f_misc  += misc;
  prefs.putUInt("fruit", f_fruit);
  prefs.putUInt("egg"  , f_egg  );
  prefs.putUInt("prot" , f_prot );
  prefs.putUInt("prep" , f_prep );
  prefs.putUInt("misc" , f_misc );
  fruit = 0;
  egg   = 0;
  prot  = 0;
  prep  = 0;
  misc  = 0;
}

// Go to sleep - uses deep sleep if line 25 (#define DEEP_SLEEP) is uncommented;
/*
 * Perks of light sleep: 
 * wakes up from any button 
 * 
 * Cons of light sleep:
 * Doesn't seem to register which GPIO woke it up
 * Uses more current
 * 
 * Perks of deep sleep: 
 * Much less current usage
 * 
 * Cons of deep sleep: 
 * Only 3 of the GPIO pins used for the buttons can wake it up
 */
void go_sleep() {
  store();

#ifdef DEEP_SLEEP
  tft.fillScreen(TFT_GREY);
  tft.setCursor(25,50);
  tft.println("Entering Sleep");
  delay(1000);
#endif
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  esp_sleep_enable_ext1_wakeup(static_cast<gpio_num_t>(PINS), ESP_EXT1_WAKEUP_ANY_HIGH);

#ifdef DEEP_SLEEP
  // DEEP SLEEP //
  esp_deep_sleep_start();
#else 
  // LIGHT SLEEP //
  gpio_wakeup_enable(static_cast<gpio_num_t>(PINS), GPIO_INTR_HIGH_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  esp_light_sleep_start();
#endif
}

// Checks the input for each button
void button_loop() 
{
  b_fruitveg.loop();
  b_eggdairy.loop();
  b_proteins.loop();
  b_prepared.loop();
  b_miscella.loop();
  b_clear.loop();
  btm.loop();
  top.loop();
}

// On wakeup, will read which button it was woken up by. Only seems to work with deep sleep???
///TODO: make light sleep display which button woke it up
void wakeup() {
  static bool once = false;
  //if (!once) {
  uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
  uint64_t pin = (log(GPIO_reason))/log(2);
  string output;
  switch (pin) { 
    case FRUITVEG:
      ++fruit;
      output = "fruit";
      break;
    case EGGDAIRY:
      ++egg;
      output = "dairy";
      break;
    case PROTEINS:
      ++prot;
      output = "protein";
      break;
    case PREPARED:
      ++prep;
      output = "prep";
      break;
    case MISCELLA:
      output = "misc";
      ++misc;
      break;
    case CLEARBTT:
      output = "clear";
      break;
    default:
      output = "Unkown";
      break;
  }
  tft.fillScreen(TFT_GREY); // Clear the screen
  tft.setCursor(0,0,4);         // Set cursor to front, use font #4
  tft.setTextColor(TFT_RED);  // Set color to be red
  tft.print("Wake by: " ); tft.println(output.c_str());
  tft.print("Battery (V): "); tft.println(battery_level);
  tft.print("Solar (V): "  ); tft.println(solar_level);
  delay(1000);
  tft.fillScreen(TFT_GREY); // Clear the screen
  tft.setTextColor(TFT_WHITE);  // Set color to be white
}

void setup() {
  // Create a namespace named 'values' in the preferences object;
  prefs.begin("values", false);
  
  /*
  ADC_EN is the ADC detection enable port
  If the USB port is used for power supply, it is turned on by default.
  If it is powered by battery, it needs to be set to high level
  */
  //pinMode(ADC_EN, OUTPUT);
  //digitalWrite(ADC_EN, HIGH);
  pinMode(BATTHIGH, OUTPUT);
  pinMode(BATT_LOW, OUTPUT);
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
  f_fruit = prefs.getUInt("fruit", f_fruit);
  f_egg   = prefs.getUInt("egg"  , f_egg  );
  f_prot  = prefs.getUInt("prot" , f_prot );
  f_prep  = prefs.getUInt("prep" , f_prep );
  f_misc  = prefs.getUInt("misc" , f_misc );
  int col = 195; // The column to align all the numbers to 
  tft.print(" Miscellaneous: " ); tft.setCursor(col,tft.getCursorY()); tft.println(f_misc  + misc );
  tft.print(" Meat & Poultry: "); tft.setCursor(col,tft.getCursorY()); tft.println(f_prot  + prot );
  tft.print(" Fruit & Veg.: "  ); tft.setCursor(col,tft.getCursorY()); tft.println(f_fruit + fruit);
  tft.print(" Prepared Meals: "); tft.setCursor(col,tft.getCursorY()); tft.println(f_prep  + prep );
  tft.print(" Egg & Dairy: "   ); tft.setCursor(col,tft.getCursorY()); tft.println(f_egg   + egg  );
  if (init_ == 10) {
    tft.fillScreen(TFT_GREY); // Clear the screen 
    wakeup();
    ++init_;
  } else {
    ++init_;
  }

  battery_level = analogReadMilliVolts(ADC_BAT) / 1000.0;
  solar_level = analogReadMilliVolts(ADC_SOL) / 1000.0;

  if (battery_level < 3.0) {  
    digitalWrite(BATTHIGH, LOW);
    digitalWrite(BATT_LOW, HIGH);
  } else {
    digitalWrite(BATTHIGH, HIGH);
    digitalWrite(BATT_LOW, LOW);
  }
  
  // Hard reset handler: 
  if (clear1 && clear2 && clear3) {
    clear1 = false;
    clear2 = false;
    clear3 = false;
    hardreset();
  }

  // Button loop
  if (btnClick) {
    btnClick = false; // reset trigger state after it was pressed
    tft.fillScreen(TFT_GREY); // Clear the screen 
    counter = 0;
  }
  ++counter;
  if (counter <= TIME_AWAKE){
    button_loop(); // Loop through and poll the buttons.
    // So that hard reset handler isn't just set off by accident 
    clear1 = false;
    clear2 = false;
    clear3 = false;
  } else {
    counter = 0;
    go_sleep();
    wakeup();
  }
}
