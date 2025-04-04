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


Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
sensors_event_t humidity, temp;

#include<Wire.h>
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
#define sleeptimeSecs 300
#define maxArray 501


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

 RTC_DATA_ATTR   int firstrun = 100;
 RTC_DATA_ATTR   int page = 2;
float dewpoint;
 float minVal = 3.9;
 float maxVal = 4.2;
RTC_DATA_ATTR int readingCount = 0; // Counter for the number of readings
int readingTime;
bool buttonstart = false;

#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=5*/ SS, /*DC=*/ 21, /*RES=*/ 20, /*BUSY=*/ 10)); // GDEH0154D67 200x200, SSD1681


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
      pinMode(SS, INPUT );
      pinMode(6, INPUT );
      pinMode(4, INPUT );
      pinMode(8, INPUT );
      pinMode(9, INPUT );
      pinMode(1, INPUT_PULLUP );
      pinMode(2, INPUT_PULLUP );
      pinMode(3, INPUT_PULLUP );
      pinMode(0, INPUT_PULLUP );
      pinMode(5, INPUT_PULLUP );



      uint64_t bitmask = BUTTON_PIN_BITMASK(GPIO_NUM_1) | BUTTON_PIN_BITMASK(GPIO_NUM_2) | BUTTON_PIN_BITMASK(GPIO_NUM_3) | BUTTON_PIN_BITMASK(GPIO_NUM_0) | BUTTON_PIN_BITMASK(GPIO_NUM_5);

      esp_deep_sleep_enable_gpio_wakeup(bitmask, ESP_GPIO_WAKEUP_GPIO_LOW);
      esp_sleep_enable_timer_wakeup(sleeptimeSecs * 1000000ULL);
      delay(1);
      esp_deep_sleep_start();
      //esp_light_sleep_start();
      delay(1000);
}


void initTime(String timezone){
  configTzTime(timezone.c_str(), "time.cloudflare.com", "pool.ntp.org", "time.nist.gov");



}



void startWifi(){

  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setCursor(0, 0);
  display.firstPage();

  do {
    display.print("Connecting...");
  } while (display.nextPage());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);  
  WiFi.setTxPower (WIFI_POWER_8_5dBm);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() > 10000) { display.print("!");}
    if ((millis() > 20000)) {break;}
    //do {
      display.print(".");
       display.display(true);
    //} while (display.nextPage());
    delay(1000);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    display.print("Connected. Getting time...");
  }
  else
  {
    display.print("Connection timed out. :(");
  }
  display.display(true);
  initTime("EST5EDT,M3.2.0,M11.1.0");


    struct tm timeinfo;
    getLocalTime(&timeinfo);
    time_t now = time(NULL);
  localtime_r(&now, &timeinfo);

  // Allocate a char array for the time string
  char timeString[10]; // "12:34 PM" is 8 chars + null terminator


}

void startWebserver(){

  //display.clearScreen();
  display.setFont();
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setCursor(0, 0);
  display.firstPage();

  do {
    display.print("Connecting...");
  } while (display.nextPage());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);  
  WiFi.setTxPower (WIFI_POWER_8_5dBm);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() > 20000) { display.print("!"); }
    if ((millis() > 30000)) {gotosleep();}
    //do {
      display.print(".");
       display.display(true);
    //} while (display.nextPage());
    delay(1000);
  }
  wipeScreen();
  display.setCursor(0, 0);
  display.firstPage();
  do {
    display.print("Connected! to: ");
    display.println(WiFi.localIP());
  } while (display.nextPage());
  ArduinoOTA.setHostname("neighbourdisp");
  ArduinoOTA.begin();
  display.println("ArduinoOTA started");
    display.print("RSSI: ");
    display.println(WiFi.RSSI());
   display.display(true);
}



void wipeScreen(){

    display.setPartialWindow(0, 0, display.width(), display.height());

    display.firstPage();
    do {
      display.fillRect(0,0,display.width(),display.height(),GxEPD_BLACK);
    } while (display.nextPage());
    delay(10);
    display.firstPage();
    do {
      display.fillRect(0,0,display.width(),display.height(),GxEPD_WHITE);
    } while (display.nextPage());
    display.firstPage();


}

void setupChart() {
  display.setTextSize(1);
  display.setFont();
    display.setCursor(0, 0);
    display.print("<");
    display.print(maxVal, 3);
    
    // Adjusted for bottom of 200x200 display
    display.setCursor(0, 193);
    display.print("<");
    display.print(minVal, 3);

    // Adjusted for horizontal placement of the additional text
    display.setCursor(100, 193);
    display.print("<#");
    display.print(readingCount - 1, 0);
    display.print("*");
    display.print(sleeptimeSecs, 0);
    display.print("s>");
    int barx = mapf(vBat, 3.3, 4.15, 0, 19); // Map battery value to progress bar width
    if (barx > 19) { barx = 19; }
    if (barx < 0) { barx = 0; }
    // Adjusted rectangle and progress bar to fit within bounds
    display.drawRect(179, 192, 19, 7, GxEPD_BLACK); // Rectangle moved to fit fully within 200x200
    display.fillRect(179, 192, barx, 7, GxEPD_BLACK); // Progress bar inside the rectangle

    // Adjusted marker lines to stay within bounds
    display.drawLine(198, 193, 198, 198, GxEPD_BLACK); 
    display.drawLine(199, 193, 199, 198, GxEPD_BLACK);

    // Set the cursor for additional chart decorations
    display.setCursor(80, 0); 
}



double mapf(float x, float in_min, float in_max, float out_min, float out_max){
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
    float yScale = 199.0 / (maxVal - minVal); // Adjusted for full vertical range
    float xStep = 200.0 / (readingCount - 1); // Adjusted for full horizontal range

    wipeScreen();

    do {
        display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

        for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
            int x0 = (i - (maxArray - readingCount)) * xStep;
            int y0 = 199 - ((array1[i] - minVal) * yScale); // Full vertical range
            int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
            int y1 = 199 - ((array1[i + 1] - minVal) * yScale); // Full vertical range

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
    } while (display.nextPage());

    display.setFullWindow();
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
    float yScale = 199.0 / (maxVal - minVal); // Adjusted for full vertical range
    float xStep = 200.0 / (readingCount - 1); // Adjusted for full horizontal range

    wipeScreen();

    do {
        display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

        for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
            int x0 = (i - (maxArray - readingCount)) * xStep;
            int y0 = 199 - ((array2[i] - minVal) * yScale); // Adjusted for vertical range
            int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
            int y1 = 199 - ((array2[i + 1] - minVal) * yScale); // Adjusted for vertical range
            if (array2[i] > 0) {
                display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
            }
        }

        setupChart();
        display.print("[");
        display.print("Hum: ");
        display.print(array2[(maxArray - 1)], 2);
        display.print("g");
        display.print("]");
    } while (display.nextPage());

    display.setFullWindow();
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
    float yScale = 199.0 / (maxVal - minVal); // Adjusted for full vertical range
    float xStep = 200.0 / (readingCount - 1); // Adjusted for full horizontal range

    wipeScreen();

    do {
        display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

        for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
            int x0 = (i - (maxArray - readingCount)) * xStep;
            int y0 = 199 - ((array3[i] - minVal) * yScale); // Adjusted for vertical range
            int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
            int y1 = 199 - ((array3[i + 1] - minVal) * yScale); // Adjusted for vertical range
            if (array3[i] > 0) {
                display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
            }
        }

        setupChart();
        display.print("[");
        display.print("Pres: ");
        display.print(array3[(maxArray - 1)], 2);
        display.print("mb");
        display.print("]");
    } while (display.nextPage());

    display.setFullWindow();
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
    float yScale = 199.0 / (maxVal - minVal); // Adjusted for full vertical range
    float xStep = 200.0 / (readingCount - 1); // Adjusted for full horizontal range

    wipeScreen();

    do {
        display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

        for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
            int x0 = (i - (maxArray - readingCount)) * xStep;
            int y0 = 199 - ((array4[i] - minVal) * yScale); // Adjusted for vertical range
            int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
            int y1 = 199 - ((array4[i + 1] - minVal) * yScale); // Adjusted for vertical range
            if (array4[i] > 0) {
                display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
            }
        }

        display.setCursor(0, 0);
        display.print("<");
        display.print(maxVal, 3);
        display.setCursor(0, 193); // Adjusted to stay 7px from the bottom
        display.print("<");
        display.print(minVal, 3);
        display.setCursor(120, 193); // Adjusted for new width
        display.print("<#");
        display.print(readingCount - 1, 0);
        display.print("*");
        display.print(sleeptimeSecs, 0);
        display.print("s>");
        display.setCursor(175, 193); // Adjusted for new width

        int batPct = mapf(vBat, 3.3, 4.15, 0, 100);
        display.setCursor(80, 0); // Same placement
        display.print("[vBat: ");
        display.print(vBat, 3);
        display.print("v/");
        display.print(batPct, 1);
        display.print("%]");
    } while (display.nextPage());

    display.setFullWindow();
    gotosleep();
}



void takeSamples(){

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
        array2[(maxArray - 1)] = h;



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
      int valueY = quadY + ((QUAD_HEIGHT + h) / 2); // +10 for visual adjustment
      display.setCursor(valueX, valueY);
      display.print(value);

      // For the small unit - position below centered value
      display.setFont(FONT1);
      display.getTextBounds(unit, 0, 0, &x1, &y1, &w, &h);
      int unitX = quadX + (QUAD_WIDTH - w) / 2;
      int unitY = valueY + 20; // Fixed distance below value
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
  int barx = mapf(vBat, 3.3, 4.15, 0, 19);
  if (barx > 19) { barx = 19; }
  display.drawRect(179, 192, 19, 7, GxEPD_BLACK);
  display.fillRect(179, 192, barx, 7, GxEPD_BLACK);
  display.drawLine(198, 193, 198, 198, GxEPD_BLACK);
  display.drawLine(199, 193, 199, 198, GxEPD_BLACK);
  display.drawRect(0, 0, display.width(), display.height(), GxEPD_BLACK);
  
  display.display(true);
}






void setup()
{
  Wire.begin();  

  vBat = analogReadMilliVolts(0) / 500.0;
  while (vBat < 2){
    vBat = analogReadMilliVolts(0) / 500.0;
    delay(10);
  }
  GPIO_reason = log(esp_sleep_get_gpio_wakeup_status())/log(2);
  

  aht.begin();
  bmp.begin();
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500);
  bmp.takeForcedMeasurement();
  
  aht.getEvent(&humidity, &temp);
   t = temp.temperature;
   h = humidity.relative_humidity;
   pres = bmp.readPressure() / 100.0;
    dewpoint = t - ((100 - h)/5);



  display.init(115200, false, 10, false); // void init(uint32_t serial_diag_bitrate, bool initial, uint16_t reset_duration = 10, bool pulldown_rst_mode = false)
  display.setRotation(2);
  display.setFont();
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  pinMode(5, INPUT_PULLUP);



   
  delay(10);

            
  if (firstrun >= 100) {display.clearScreen();
   if (page == 2){
        wipeScreen();

   }
  firstrun = 0;}
  firstrun++;

  display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
  



  if (GPIO_reason < 0) {
    //startWifi();
    takeSamples();
      switch (page){
        case 0: 
          doTempDisplay();
          break;
        case 1: 
          doTempDisplay();
          break;
        case 2: 
          doMainDisplay();
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
      while (!digitalRead(1))
        {
          delay(10);
          if (millis() > 2000) {
            startWebserver();
          return;}
        }
      page = 1;
      doTempDisplay();
      break;
    case 3: 
      page = 2;
        //wipeScreen();
        doMainDisplay();
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
      takeSamples();
      display.clearScreen();
      switch (page){
        case 0: 
          doTempDisplay();
          break;
        case 1: 
          doTempDisplay();
          break;
        case 2: 
          doMainDisplay();
          break;
        case 3: 
          doHumDisplay();
          break;
        case 4: 
          doBatDisplay();
          break;
      }
  }

  

}

void loop()
{
ArduinoOTA.handle();
if (!digitalRead(0)) {gotosleep();}
delay(250);

}
