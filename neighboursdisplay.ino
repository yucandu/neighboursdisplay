#include <GxEPD2_BW.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include <AsyncElegantOTA.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include "bmos.h"
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
sensors_event_t humidity, temp;

#include <Wire.h>
#define I2C_ADDRESS 0x48

#include "driver/periph_ctrl.h"
int GPIO_reason;
#include "esp_sleep.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/Open_Sans_Condensed_Bold_48.h>

#define FONT1 &FreeSans9pt7b
#define FONT2 &Open_Sans_Condensed_Bold_48



const char* ssid = "mikesnet";
bool isSetNtp = false;
const char* password = "springchicken";

#define ENABLE_GxEPD2_GFX 1
#define TIME_TIMEOUT 20000
int sleeptimeSecs = 300;
#define maxArray 501

#define MENU_MAX 7
int menusel = 1;
bool editinterval = false;
Preferences preferences;
RTC_DATA_ATTR bool showDewpoint = false;  // false = humidity, true = dewpoint
RTC_DATA_ATTR bool antiGhosting = false;  // false = humidity, true = dewpoint
WiFiManager wm;
// Add with other globals at top
RTC_DATA_ATTR int displayRotation = 2;  // 1-4 for display rotation
RTC_DATA_ATTR int bmo = 1;  // 1-3
RTC_DATA_ATTR int tickRate = 12;  // 1-3
bool editTickRate = false;  // For editing mode
bool editRotation = false;              // For editing rotation in menu
RTC_DATA_ATTR float array1[maxArray];
RTC_DATA_ATTR float array2[maxArray];
RTC_DATA_ATTR float array3[maxArray];
RTC_DATA_ATTR float array4[maxArray];
RTC_DATA_ATTR float windspeed, windgust, fridgetemp, outtemp;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;  //Replace with your GMT offset (secs)
const int daylightOffset_sec = 0;   //Replace with your daylight offset (secs)
float t, h, pres, barx;
float v41_value, v42_value, v62_value;

RTC_DATA_ATTR int screenCounter = 100;
RTC_DATA_ATTR int page = 0;
float dewpoint;
float minVal = 3.9;
float maxVal = 4.2;
RTC_DATA_ATTR int readingCount = 0;  // Counter for the number of readings
int readingTime;
bool buttonstart = false;

#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=5*/ SS, /*DC=*/21, /*RES=*/20, /*BUSY=*/10));  // GDEH0154D67 200x200, SSD1681


const char* bedroomauth = "8_-CN2rm4ki9P3i_NkPhxIbCiKd5RXhK";  //hubert

const char* v41_pin = "V41";
const char* v62_pin = "V62";

float vBat;

float findLowestNonZero(float a, float b, float c) {
  // Initialize minimum to a very large value
  float minimum = 999;

  // Check each variable and update the minimum value
  if (a != 0.0 && a < minimum) {
    minimum = a;
  }
  if (b != 0.0 && b < minimum) {
    minimum = b;
  }
  if (c != 0.0 && c < minimum) {
    minimum = c;
  }

  return minimum;
}

void gotosleep() {
  WiFi.disconnect();
  display.hibernate();
  //SPI.end();
  Wire.end();
  pinMode(SS, INPUT);
  pinMode(6, INPUT);
  pinMode(4, INPUT);
  pinMode(8, INPUT);
  pinMode(9, INPUT);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(0, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);



  uint64_t bitmask = BUTTON_PIN_BITMASK(GPIO_NUM_1) | BUTTON_PIN_BITMASK(GPIO_NUM_2) | BUTTON_PIN_BITMASK(GPIO_NUM_3) | BUTTON_PIN_BITMASK(GPIO_NUM_0) | BUTTON_PIN_BITMASK(GPIO_NUM_5);

  esp_deep_sleep_enable_gpio_wakeup(bitmask, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_sleep_enable_timer_wakeup(sleeptimeSecs * 1000000ULL);
  delay(1);
  esp_deep_sleep_start();
  //esp_light_sleep_start();
  delay(1000);
}


void initTime(String timezone) {
  configTzTime(timezone.c_str(), "time.cloudflare.com", "pool.ntp.org", "time.nist.gov");
}



void startWifi() {

  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setCursor(0, 0);
  display.firstPage();

  do {
    display.print("Connecting...");
  } while (display.nextPage());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() > 10000) { display.print("!"); }
    if ((millis() > 20000)) { break; }
    //do {
    display.print(".");
    display.display(true);
    //} while (display.nextPage());
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    display.print("Connected. Getting time...");
  } else {
    display.print("Connection timed out. :(");
  }
  display.display(true);
  initTime("EST5EDT,M3.2.0,M11.1.0");


  struct tm timeinfo;
  getLocalTime(&timeinfo);
  time_t now = time(NULL);
  localtime_r(&now, &timeinfo);

  // Allocate a char array for the time string
  char timeString[10];  // "12:34 PM" is 8 chars + null terminator
}

void startWebserver() {
  display.setPartialWindow(0, 0, display.width(), display.height());
  wipeScreen();
  display.setCursor(0, 0);  //
  display.println("Use your mobile phone to connect to ");
  display.println("[BMO Setup] then browse to");
  display.println("http://192.168.4.1 to connect to WiFi");
  display.display(true);
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  wm.setConfigPortalTimeout(180);
  bool res;
  res = wm.autoConnect("BMO Setup");
  if (res) {
    ArduinoOTA.setHostname("neighbourdisp");
    ArduinoOTA.begin();
  }

  displayMenu();
}

void displayMenu() {
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setTextSize(1);
  display.fillScreen(GxEPD_WHITE);
  display.setCursor(0, 0);
  display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
  switch (bmo) {
    case 1:
        display.drawInvertedBitmap(40, 60, epd_bitmap_bmomenu1, 112, 120, GxEPD_BLACK);
        break;
    case 2:
        display.drawInvertedBitmap(40, 60, epd_bitmap_bmomenu2, 123, 124, GxEPD_BLACK);
        break;
    case 3:
        display.drawInvertedBitmap(55, 60, epd_bitmap_bmomenu3, 93, 91, GxEPD_BLACK);
        break;
  }



  if (WiFi.status() == WL_CONNECTED) {
    display.setCursor(0, 170);
    display.print("Connected to: ");
    display.println(WiFi.localIP());
    display.print("RSSI: ");
    display.println(WiFi.RSSI());
    display.println("");
  }
    display.setCursor(0, 0);

    // Menu items with right-justified values
    if (menusel == 1) {display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);} else {display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);}
    display.println("Start WifiManager");

    // Change Interval
    if (menusel == 2) {display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);} else {display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);}
    display.print("Change Interval");
    display.setCursor(160, 8);  // Right justify
    display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);  // Reset color for value
    if (editinterval) {display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);}  // Highlight only when editing
    display.print(sleeptimeSecs);
    display.println("s");

    // Toggle Chart
    if (menusel == 3) {display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);} else {display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);}
    display.print("Toggle Chart");
    display.setCursor(160, 8*2);  // Right justify
    display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);  // Reset color for value
    display.println(showDewpoint ? "D" : "H");

    // Set Rotation
    if (menusel == 4) {display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);} else {display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);}
    display.print("Set Rotation");
    display.setCursor(160, 8*3);  // Right justify
    display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);  // Reset color for value
    if (editRotation) {display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);}  // Highlight only when editing
    display.println(displayRotation);

    // Anti-Ghost
    if (menusel == 5) {display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);} else {display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);}
    display.print("Anti-Ghost");
    display.setCursor(160, 8*4);  // Right justify
    display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);  // Reset color for value
    display.println(antiGhosting ? "Y" : "N");

    // Tick Rate
    if (menusel == 6) {display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);} else {display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);}
    display.print("Tick Rate");
    display.setCursor(160, 8*5);  // Right justify
    display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);  // Reset color for value
    if (editTickRate) {display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);}  // Highlight only when editing
    display.println(tickRate);

    // Exit
    if (menusel == 7) {display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);} else {display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);}
    display.println("Exit");


    // Status area at bottom
    display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
    display.setCursor(0, 190);  // Move sensor data to bottom
    display.print("T:");
    display.print(t, 1);
    display.print("C H:");
    display.print(h, 1);
    display.print("% P:");
    display.print(pres, 0);
    display.print("hPa B:");
    display.print(vBat, 2);
    display.print("v");

    display.display(true);
}

void wipeScreen() {
  if (antiGhosting) {
    display.setPartialWindow(0, 0, display.width(), display.height());

    display.firstPage();
    do {
      display.fillRect(0, 0, display.width(), display.height(), GxEPD_BLACK);
    } while (display.nextPage());
    delay(10);
    display.firstPage();
    do {
      display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);
    } while (display.nextPage());
    display.firstPage();
  }
  else {display.fillScreen(GxEPD_WHITE);}
}

void setupChart() {
  display.setTextSize(1);
  display.setFont();
  
  // Draw Y-axis ticks - 9 ticks creating 10 spaces
  for(int i = 0; i <= 200; i+=20) {
      display.drawLine(0, i, 3, i, GxEPD_BLACK);  // 3 pixel wide ticks
  }
  display.drawLine(0, 0, 3, 0, GxEPD_BLACK);  // 3 pixel wide ticks
  display.drawLine(0, 199, 3, 199, GxEPD_BLACK);  // 3 pixel wide ticks
  
  // Draw X-axis ticks - one every 10 data points
  int numTicks = (readingCount - 1) / tickRate;
  if(numTicks > 0) {
      float xStep = 200.0 / numTicks;
      for(int i = 1; i < numTicks; i++) {
          int x = i * xStep;
          display.drawLine(x, 189, x, 192, GxEPD_BLACK);
      }
  }

  // Rest of original chart decorations
  display.setCursor(4, 0);
  display.print(maxVal, 3);

  display.setCursor(4, 193);
  display.print(minVal, 3);

  // Battery and other indicators remain unchanged
  display.setCursor(70, 193);
  display.print("<#");
  display.print(readingCount - 1, 0);
  display.print("*");
  display.print(sleeptimeSecs, 0);
  display.print("s>");
  
  int barx = mapf(vBat, 3.3, 4.1, 0, 19);
  if (barx > 19) { barx = 19; }
  if (barx < 0) { barx = 0; }
  display.drawRect(179, 192, 19, 7, GxEPD_BLACK);
  display.fillRect(179, 192, barx, 7, GxEPD_BLACK);
  display.drawLine(198, 193, 198, 198, GxEPD_BLACK);
  display.drawLine(199, 193, 199, 198, GxEPD_BLACK);

  display.setCursor(80, 0);
}



double mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void doTempDisplay() {
  // Recalculate min and max values
  minVal = array1[maxArray - readingCount];
  maxVal = array1[maxArray - readingCount];

  for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
    if (array1[i] != 0) {  // Only consider non-zero values
      if (array1[i] < minVal) {
        minVal = array1[i];
      }
      if (array1[i] > maxVal) {
        maxVal = array1[i];
      }
    }
  }

  // Calculate scaling factors for full 200x200 display
  float yScale = 199.0 / (maxVal - minVal);  // Adjusted for full vertical range
  float xStep = 200.0 / (readingCount - 1);  // Adjusted for full horizontal range

  wipeScreen();

  //do {
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

    for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
      int x0 = (i - (maxArray - readingCount)) * xStep;
      int y0 = 199 - ((array1[i] - minVal) * yScale);  // Full vertical range
      int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
      int y1 = 199 - ((array1[i + 1] - minVal) * yScale);  // Full vertical range

      // Only draw a line for valid (non-zero) values
      if (array1[i] != 0) {
        display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
      }
    }

    // Call setupChart to draw chart decorations
    setupChart();

    // Display temperature information at the bottom
    display.print("[");
    display.print("Temp: ");
    display.print(array1[maxArray - 1], 3);
    display.print(char(247));
    display.print("c");
    display.print("]");
 // } while (display.nextPage());

  display.display(true);
  gotosleep();
}


void doHumDisplay() {
  // Recalculate min and max values
  minVal = array2[maxArray - readingCount];
  maxVal = array2[maxArray - readingCount];

  for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
    if ((array2[i] < minVal) && (array2[i] > 0)) {
      minVal = array2[i];
    }
    if (array2[i] > maxVal) {
      maxVal = array2[i];
    }
  }

  // Adjust scaling factors
  float yScale = 199.0 / (maxVal - minVal);  // Adjusted for full vertical range
  float xStep = 200.0 / (readingCount - 1);  // Adjusted for full horizontal range

  wipeScreen();

 // do {
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

    for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
      int x0 = (i - (maxArray - readingCount)) * xStep;
      int y0 = 199 - ((array2[i] - minVal) * yScale);  // Adjusted for vertical range
      int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
      int y1 = 199 - ((array2[i + 1] - minVal) * yScale);  // Adjusted for vertical range
      if (array2[i] > 0) {
        display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
      }
    }

    setupChart();

    display.print("[");
    if (!showDewpoint) {
      display.print("Hum: ");
      display.print(array2[(maxArray - 1)], 2);
      display.print("%");
    } else {
      display.print("Dewp: ");
      display.print(array2[(maxArray - 1)], 2);
      display.print(char(247));
      display.print("c");
      display.print("");
    }
    display.print("]");
 // } while (display.nextPage());

  display.display(true);
  gotosleep();
}

void doMainDisplay() {
  wipeScreen();
  updateMain();
  gotosleep();
}

void doPresDisplay() {
  // Recalculate min and max values
  minVal = array3[maxArray - readingCount];
  maxVal = array3[maxArray - readingCount];

  for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
    if ((array3[i] < minVal) && (array3[i] > 0)) {
      minVal = array3[i];
    }
    if (array3[i] > maxVal) {
      maxVal = array3[i];
    }
  }

  // Adjust scaling factors
  float yScale = 199.0 / (maxVal - minVal);  // Adjusted for full vertical range
  float xStep = 200.0 / (readingCount - 1);  // Adjusted for full horizontal range

  wipeScreen();

  //do {
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

    for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
      int x0 = (i - (maxArray - readingCount)) * xStep;
      int y0 = 199 - ((array3[i] - minVal) * yScale);  // Adjusted for vertical range
      int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
      int y1 = 199 - ((array3[i + 1] - minVal) * yScale);  // Adjusted for vertical range
      if (array3[i] > 0) {
        display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
      }
    }

    setupChart();
    display.print("[");
    display.print("Pres: ");
    display.print(array3[(maxArray - 1)], 2);
    display.print("hPa");
    display.print("]");
  //} while (display.nextPage());

  display.display(true);
  gotosleep();
}


void doBatDisplay() {
  display.setFont();
  // Recalculate min and max values
  display.setTextSize(1);
  minVal = array4[maxArray - readingCount];
  maxVal = array4[maxArray - readingCount];

  for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
    if ((array4[i] < minVal) && (array4[i] > 0)) {
      minVal = array4[i];
    }
    if (array4[i] > maxVal) {
      maxVal = array4[i];
    }
  }

  // Adjust scaling factors
  float yScale = 199.0 / (maxVal - minVal);  // Adjusted for full vertical range
  float xStep = 200.0 / (readingCount - 1);  // Adjusted for full horizontal range

  wipeScreen();

  //do {
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

    // Draw Y-axis ticks - 9 ticks creating 10 spaces
    for(int i = 0; i <= 200; i+=20) {
        display.drawLine(0, i, 3, i, GxEPD_BLACK);  // 3 pixel wide ticks
    }
    display.drawLine(0, 0, 3, 0, GxEPD_BLACK);  // 3 pixel wide ticks
    display.drawLine(0, 199, 3, 199, GxEPD_BLACK);  // 3 pixel wide ticks
    
    // Draw X-axis ticks - one every 10 data points
    int numTicks = (readingCount - 1) / tickRate;
    if(numTicks > 0) {
        float xStep = 200.0 / numTicks;
        for(int i = 1; i < numTicks; i++) {
            int x = i * xStep;
            display.drawLine(x, 189, x, 192, GxEPD_BLACK);
        }
    }

    // Rest of original chart decorations
    display.setCursor(4, 0);
    display.print(maxVal, 3);

    display.setCursor(4, 193);
    display.print(minVal, 3);

    for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
      int x0 = (i - (maxArray - readingCount)) * xStep;
      int y0 = 199 - ((array4[i] - minVal) * yScale);  // Adjusted for vertical range
      int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
      int y1 = 199 - ((array4[i + 1] - minVal) * yScale);  // Adjusted for vertical range
      if (array4[i] > 0) {
        display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
      }
    }


    display.setCursor(70, 193);  // Adjusted for new width
    display.print("<#");
    display.print(readingCount - 1, 0);
    display.print("*");
    display.print(sleeptimeSecs, 0);
    display.print("s>");


    int batPct = mapf(vBat, 3.3, 4.1, 0, 100);
    display.setCursor(80, 0);  // Same placement
    display.print("[vBat: ");
    display.print(vBat, 3);
    display.print("v/");
    display.print(batPct, 1);
    display.print("%]");
  //} while (display.nextPage());

  display.display(true);
  gotosleep();
}



void recordSensors() {

  for (int i = 0; i < (maxArray - 1); i++) {
    array3[i] = array3[i + 1];
  }
  array3[(maxArray - 1)] = pres;



  if (readingCount < maxArray) {
    readingCount++;
  }

  for (int i = 0; i < (maxArray - 1); i++) {
    array1[i] = array1[i + 1];
  }
  array1[(maxArray - 1)] = t;

  for (int i = 0; i < (maxArray - 1); i++) {
    array2[i] = array2[i + 1];
  }
  if (!showDewpoint) {
    array2[(maxArray - 1)] = h;
  } else {
    array2[(maxArray - 1)] = dewpoint;
  }



  for (int i = 0; i < (maxArray - 1); i++) {
    array4[i] = array4[i + 1];
  }
  array4[(maxArray - 1)] = vBat;
}

void updateMain() {
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);

  // Draw quadrant lines
  display.drawLine(99, 0, 99, 200, GxEPD_BLACK);
  display.drawLine(100, 0, 100, 200, GxEPD_BLACK);
  display.drawLine(0, 99, 200, 99, GxEPD_BLACK);
  display.drawLine(0, 100, 200, 100, GxEPD_BLACK);

  const int QUAD_WIDTH = 100;
  const int QUAD_HEIGHT = 100;

  auto centerTextInQuad = [&](int quadX, int quadY, const char* value, const char* unit, const char* title) {
    // For the title at top
    display.setFont(FONT1);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
    int titleX = quadX + (QUAD_WIDTH - w) / 2;
    display.setCursor(titleX, quadY + 15);
    display.print(title);

    // For the large value - get bounds first to center vertically
    display.setFont(FONT2);
    display.getTextBounds(value, 0, 0, &x1, &y1, &w, &h);
    int valueX = quadX + (QUAD_WIDTH - w) / 2;
    // Center value vertically in remaining space (after title)
    int valueY = quadY + ((QUAD_HEIGHT + h) / 2);  // +10 for visual adjustment
    display.setCursor(valueX, valueY);
    display.print(value);

    // For the small unit - position below centered value
    display.setFont(FONT1);
    display.getTextBounds(unit, 0, 0, &x1, &y1, &w, &h);
    int unitX = quadX + (QUAD_WIDTH - w) / 2;
    int unitY = valueY + 20;  // Fixed distance below value
    display.setCursor(unitX, unitY);
    display.print(unit);
  };

  // Prepare value strings
  char tempStr[10], humStr[10], presStr[10], dewStr[10];
  snprintf(tempStr, sizeof(tempStr), "%.1f", t);
  snprintf(humStr, sizeof(humStr), "%.1f", h);
  snprintf(presStr, sizeof(presStr), "%.0f", pres);
  snprintf(dewStr, sizeof(dewStr), "%.1f", dewpoint);

  // Draw each quadrant with title, value and unit
  centerTextInQuad(0, 0, tempStr, "°C", "Temp");
  centerTextInQuad(100, 0, humStr, "%", "RH");
  centerTextInQuad(0, 100, presStr, "hPa", "Pressure");
  centerTextInQuad(100, 100, dewStr, "°C", "Dewpoint");

  // Battery status (unchanged)
  display.setFont();
  int barx = mapf(vBat, 3.3, 4.1, 0, 19);
  if (barx > 19) { barx = 19; }
  display.drawRect(179, 192, 19, 7, GxEPD_BLACK);
  display.fillRect(179, 192, barx, 7, GxEPD_BLACK);
  display.drawLine(198, 193, 198, 198, GxEPD_BLACK);
  display.drawLine(199, 193, 199, 198, GxEPD_BLACK);
  //display.drawRect(0, 0, display.width(), display.height(), GxEPD_BLACK);

  display.display(true);
}


void batCheck() {
  if (vBat < 3.4) {
    wipeScreen();
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);
    display.setCursor(10, 60);
    display.setFont(FONT2);
    display.setTextSize(1);
    display.print("LOW BATTERY");
    display.display(true);
    gotosleep();
  }
}

void doSesnors() {
  bmp.takeForcedMeasurement();
  aht.getEvent(&humidity, &temp);
  t = temp.temperature;
  h = humidity.relative_humidity;
  pres = bmp.readPressure() / 100.0;
  dewpoint = t - ((100 - h) / 5);
}



void setup() {
  Wire.begin();
  preferences.begin("my-app", false);
    sleeptimeSecs = preferences.getInt("sleeptimeSecs", 300);
    showDewpoint = preferences.getBool("showDewpoint", false);
    antiGhosting = preferences.getBool("antiGhost", false);
    displayRotation = preferences.getInt("rotation", 2);
    tickRate = preferences.getInt("tickRate", 12);  // Load tickrate
  preferences.end();
  vBat = analogReadMilliVolts(0) / 500.0;
  while (vBat < 2) {
    vBat = analogReadMilliVolts(0) / 500.0;
    delay(10);
  }
  batCheck();
  GPIO_reason = log(esp_sleep_get_gpio_wakeup_status()) / log(2);


  aht.begin();
  bmp.begin();
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,  /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,  /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16, /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,   /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500);
  doSesnors();



  display.init(115200, false, 10, false);  // void init(uint32_t serial_diag_bitrate, bool initial, uint16_t reset_duration = 10, bool pulldown_rst_mode = false)
  display.setRotation(displayRotation);
  display.setFont();
  
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  pinMode(5, INPUT_PULLUP);




  delay(10);


  if (screenCounter >= 100) {
    display.clearScreen();
    if (page == 0) {
      wipeScreen();
    }
    screenCounter = 0;
  }
  screenCounter++;

  display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);


  recordSensors();

  if (GPIO_reason < 0) {
    //startWifi();
    switch (page) {
      case 0:
        doMainDisplay();
        break;
      case 1:
        doTempDisplay();
        break;
      case 2:
        doPresDisplay();
        break;
      case 3:
        doHumDisplay();
        break;
      case 4:
        doBatDisplay();
        break;
    }
  }
  switch (GPIO_reason) {
    case 1:
      delay(50);
      display.fillScreen(GxEPD_WHITE);
      display.print("Loading Menu...");
      while (!digitalRead(1)) {
        display.print(".");
        display.display(true);
        delay(10);
        if (millis() > 5000) {
          display.print("Okay let go!");
          display.display(true);
          while (!digitalRead(1)) { delay(1); }
          if (bmo == 3) {bmo = 1;} else {bmo++;}
          displayMenu();
          return;
        }
      }
      page = 1;
      doTempDisplay();
      break;
    case 3:
      page = 2;
      //wipeScreen();
      doPresDisplay();
      break;
    case 2:
      page = 3;
      doHumDisplay();
      break;
    case 5:
      page = 4;
      doBatDisplay();
      break;
    case 0:
      page = 0;
      doMainDisplay();
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
  }

  if (millis() > 600000) { //reboot after 10 minutes
    esp_sleep_enable_timer_wakeup(1 * 1000000ULL);
    delay(1);
    esp_deep_sleep_start();
  }

  if (!digitalRead(1)) {
    doSesnors();
    if (editRotation) {
      displayRotation++;
      if (displayRotation > 4) displayRotation = 1;
      display.setRotation(displayRotation);
    } else if (editinterval) {
      sleeptimeSecs += 30;
      if (sleeptimeSecs > 86400) sleeptimeSecs = 30;
    } else if (editTickRate) {
      tickRate += 1;
      if (tickRate > 250) tickRate = 2;
    } else {
        menusel++;
        if (menusel > MENU_MAX) menusel = 1;
    }
    displayMenu();
    delay(200);
  }

  if (!digitalRead(3)) {
    doSesnors();
    if (editRotation) {
      displayRotation--;
      if (displayRotation < 1) displayRotation = 4;
      display.setRotation(displayRotation);
    } else if (editinterval) {
      sleeptimeSecs -= 30;
      if (sleeptimeSecs < 30) sleeptimeSecs = 86400;
    } else if (editTickRate) {
      tickRate -= 1;
      if (tickRate < 2) tickRate = 250;
    } else {
        menusel--;
        if (menusel < 1) menusel = MENU_MAX;
    }
    displayMenu();
    delay(200);
  }

  if (!digitalRead(0)) {
    doSesnors();
    switch (menusel) {
      case 1:  // Start WiFiManager
        startWebserver();
        break;
      case 2:  // Edit Interval
        editinterval = !editinterval;
        if (!editinterval) {  // Save when exiting edit mode
          preferences.begin("my-app", false);
          preferences.putInt("sleeptimeSecs", sleeptimeSecs);
          preferences.end();
        }
        displayMenu();
        break;
      case 3:  // Toggle Chart Type
        showDewpoint = !showDewpoint;
        preferences.begin("my-app", false);
        preferences.putBool("showDewpoint", showDewpoint);
        preferences.end();
        displayMenu();
        break;
      case 4:  // Rotation
        editRotation = !editRotation;
        if (!editRotation) {  // Save when exiting edit mode
          preferences.begin("my-app", false);
          preferences.putInt("rotation", displayRotation);
          preferences.end();
        }
        displayMenu();
        break;
      case 5:  // Anti-ghosting
          antiGhosting = !antiGhosting;
          preferences.begin("my-app", false);
          preferences.putBool("antiGhost", antiGhosting);
          preferences.end();
          displayMenu();
          break;
      case 6:  // Tick Rate
          editTickRate = !editTickRate;
          if (!editTickRate) {  // Save when exiting edit mode
              preferences.begin("my-app", false);
              preferences.putInt("tickRate", tickRate);
              preferences.end();
          }
          displayMenu();
          break;
      case 7:  // Exit (previously case 6)
          esp_sleep_enable_timer_wakeup(1 * 1000000ULL);
          delay(1);
          esp_deep_sleep_start();
          break;
    }
    delay(200);
  }
}
