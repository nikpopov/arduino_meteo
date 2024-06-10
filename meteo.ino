// METEO HOME STATION PROJECT
/* Use Arduino NANO as microcontroller
Measure temperature and humidity by sensor DHT22
Measure pressure by sensor GY-68/BMP180
Integrated calendar and clock using RTC
All data display on LCD 20x4 (with I2C interface)

Components' needed:
ARDUINO NANO
DHT22 - temperature and humidity sensor
BMP180 - pressure sensor
DS1307RTC - Real Time Clock module
LCD 16x2 with I2C interface (can be used without I2C interface but needs to change LCD control code)
Buttons - 4 pcs (for calendar and clock set up and correction)

*                   Pin Connection Diagram
* ARDUINO | DHT22  | BMP180 | DS1307 | LCD      |  S1  |  S2  |   S3    |   S4      |
*  NANO   |        |        |        | I2C 20x4 | EDIT | SAVE | UP/NEXT | DOWN/PREV |
----------|--------|--------|--------|----------|------|------|---------|-----------|
*  +5V    | VIN(1) |        |  VCC   |  VIN     |      |      |         |           |
*   D2    | DAT(2) |        |        |          |      |      |         |           |
*   GND   | GND(4) | GND(2) |  GND   |  GND     | GND  | GND  |   GND   |   GND     |
*  +3.3V  |        | VIN(1) |        |          |      |      |         |           |
*   A5    |        | SCL(3) |  SCL   |  SCL     |      |      |         |           |
*   A4    |        | SDA(4) |  SDA   |  SDA     |      |      |         |           |
*   D3    |        |        |        |          |  +   |      |         |           |
*   D4    |        |        |        |          |      |  +   |         |           |
*   D5    |        |        |        |          |      |      |    +    |           |
*   D6    |        |        |        |          |      |      |         |     +     |
*/

#include <Wire.h>  // Include Wire library for I2C communication
#include "DHT.h"  // Include DHT library
#include "Adafruit_BMP085.h" // Include BMP-180 library
#include <Adafruit_Sensor.h>
#include <RTClib.h> // Include RTC library
#include "LiquidCrystal_I2C.h"  // Include Liquid Crystal Display 20x4 library


#define DHTPIN 2            // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22       // DHT sensor type

#define BUTTON_EDIT_PIN 3   // Define EDIT buttin pin
#define BUTTON_SAVE_PIN 4    // Define SAVE buttin pin
#define BUTTON_UP_PIN 5     // Define UP buttin pin
#define BUTTON_DOWN_PIN 6   // Define DOWN buttin pin

#define DEBOUNCE_DELAY 50   // Define debounce time
#define LONG_PRESS_DURATION 1000 // Define long press time setting for EDIT mode enter

DHT dht(DHTPIN, DHTTYPE);   // Define DHT sensor
Adafruit_BMP085 bmp;        // Define BMP sensor
RTC_DS1307 rtc;             // Define DS1307RTC
LiquidCrystal_I2C lcd(0x27, 20, 4);;  // Set the LCD address to 0x27 for a 20 chars and 4 line display

unsigned long lastDebounceTime = 0;
unsigned long lastButtonPressTime = 0;
int buttonState = HIGH;
int lastButtonState = HIGH;
bool editing = false;
int cursorPosition = 0; // 0 for day, 1 for month, 2 for year
int editModeStep = 0; // 0 for time, 1 for date
bool backlightOn = true;
unsigned long backlightTimer = 0;

// Initial time and date settings
int editHour = 0, editMinute = 0, editSecond = 0;
int editDay = 1, editMonth = 1, editYear = 2020;

void setup() {
  // Set up sensors and LCD
  while(!Serial);
  Serial.begin(9600);
  dht.begin();          // Initialize DHT22 sensor
  bmp.begin();          // Initialize BMP180 sensor
  if (!bmp.begin(0x77)) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(2020, 1, 1, 0, 0, 0));
  }

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  // lcd.print("Date: ");
  // lcd.setCursor(0, 1);
  // lcd.print("Temperature: ");
  // lcd.setCursor(19, 1);
  // lcd.print("C");
  // lcd.setCursor(0, 2);
  // lcd.print("Humidity: ");
  // lcd.setCursor(19, 2);
  // lcd.print("%");
  // lcd.setCursor(0, 3);
  // lcd.print("Pressure: ");
  // lcd.setCursor(16, 3);
  // lcd.print("mmhg");
  displayLabels();

  pinMode(BUTTON_EDIT_PIN, INPUT_PULLUP); // Initialize EDIT button and pull_up resistor
  pinMode(BUTTON_SAVE_PIN, INPUT_PULLUP); // Initialize SAVE button and pull_up resistor
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);   // Initialize UP button and pull_up resistor
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP); // Initialize DOWN button and pull_up resistor
}

void loop() {
  handleButtons();    // Handle whether EDIT mode is active
  if (editing) {
    editDateTime();   // EDIT mode
  } else {
    displayLabels();
    displaySensorData();
    displayDateTime();
  }
  handleBacklight();  // Handle LCD backlight for energy saving
  delay(200);
}

void displayLabels() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Temperature: ");
  lcd.setCursor(19, 1);
  lcd.print("C");
  lcd.setCursor(0, 2);
  lcd.print("Humidity: ");
  lcd.setCursor(19, 2);
  lcd.print("%");
  lcd.setCursor(0, 3);
  lcd.print("Pressure: ");
  lcd.setCursor(16, 3);
  lcd.print("mmhg");
}

void handleButtons() {
  int reading = digitalRead(BUTTON_EDIT_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        lastButtonPressTime = millis();
      } else {
        if ((millis() - lastButtonPressTime) > LONG_PRESS_DURATION) {
          if (!editing) {
            editing = true;
            cursorPosition = 0;
            editModeStep = 0;
            DateTime now = rtc.now();
            editHour = now.hour();
            editMinute = now.minute();
            editSecond = now.second();
            editDay = now.day();
            editMonth = now.month();
            editYear = now.year();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Edit Mode: Time");
          } else {
            saveDateTime();
            editing = false;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Temp:  Humidity:");
            lcd.setCursor(0, 2);
            lcd.print("Pressure:");
          }
        } else {
          if (editing) {
            editModeStep = (editModeStep + 1) % 2;
            if (editModeStep == 0) {
              cursorPosition = 0;
              lcd.setCursor(0, 0);
              lcd.print("Edit Mode: Time");
            } else {
              cursorPosition = 3;
              lcd.setCursor(0, 0);
              lcd.print("Edit Mode: Date");
            }
          }
        }
      }
    }
  }
  lastButtonState = reading;

  if (editing) {
    if (digitalRead(BUTTON_UP_PIN) == LOW) {
      incrementValue();
      delay(200);
    }
    if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
      decrementValue();
      delay(200);
    }
    if (digitalRead(BUTTON_SAVE_PIN) == LOW) {
      saveDateTime();
      cursorPosition++;
      if (editModeStep == 0 && cursorPosition > 2) cursorPosition = 0;
      if (editModeStep == 1 && cursorPosition > 5) cursorPosition = 3;
      delay(200);
    }
  }
}

void displaySensorData() {
  // Read temperature and humidity from DHT sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  // Read pressure from BMP-180 sensor
  float pressure = bmp.readPressure() * 0.00750062;

  if (isnan(temperature) || isnan(humidity) || isnan(pressure)) {
    // lsd.print(""Failed to read from sensors!"");
    Serial.println("Failed to read from sensors!");
    return;
  }
  // Debug section allow check sensors' functionality
  // Serial.print("Temperature: ");
  // Serial.print(temperature);
  // Serial.println(" °C");
  // Serial.print("Humidity: ");
  // Serial.print(humidity);
  // Serial.println(" %");
  // Serial.print("Pressure: ");
  // Serial.print(pressure);
  // Serial.println(" mmhg");
  
  // Display results on LCD
  lcd.setCursor(13, 1);
  lcd.print(temperature);
  lcd.setCursor(13, 2);
  lcd.print(humidity);
  lcd.setCursor(10, 3);
  lcd.print(pressure);
}

void displayDateTime() {
  DateTime now = rtc.now();
  lcd.setCursor(0, 0);
  lcd.print(now.hour() < 10 ? "0" : "");
  lcd.print(now.hour());
  lcd.print(":");
  lcd.print(now.minute() < 10 ? "0" : "");
  lcd.print(now.minute());
  lcd.print(":");
  lcd.print(now.second() < 10 ? "0" : "");
  lcd.print(now.second());
  lcd.setCursor(10, 0);
  lcd.print(now.day() < 10 ? "0" : "");
  lcd.print(now.day());
  lcd.print("-");
  lcd.print(now.month() < 10 ? "0" : "");
  lcd.print(now.month());
  lcd.print("-");
  lcd.print(now.year());
}

void handleBacklight() {
  if (editing || digitalRead(BUTTON_EDIT_PIN) == LOW || digitalRead(BUTTON_SAVE_PIN) == LOW || digitalRead(BUTTON_UP_PIN) == LOW || digitalRead(BUTTON_DOWN_PIN) == LOW) {
    lcd.backlight();
    backlightOn = true;
    backlightTimer = millis();
  }
  if (backlightOn && millis() - backlightTimer > 20000) {
    lcd.noBacklight();
    backlightOn = false;
  }
}

void incrementValue() {
  switch (cursorPosition) {
    case 0: editHour = (editHour + 1) % 24; break;
    case 1: editMinute = (editMinute + 1) % 60; break;
    case 2: editSecond = (editSecond + 1) % 60; break;
    case 3: editDay = (editDay % 31) + 1; break;
    case 4: editMonth = (editMonth % 12) + 1; break;
    case 5: editYear++; break;
  }
  blinkCursor();
}

void decrementValue() {
  switch (cursorPosition) {
    case 0: editHour = (editHour > 0) ? editHour - 1 : 23; break;
    case 1: editMinute = (editMinute > 0) ? editMinute - 1 : 59; break;
    case 2: editSecond = (editSecond > 0) ? editSecond - 1 : 59; break;
    case 3: editDay = (editDay > 1) ? editDay - 1 : 31; break;
    case 4: editMonth = (editMonth > 1) ? editMonth - 1 : 12; break;
    case 5: editYear--; break;
  }
  blinkCursor();
}

void blinkCursor() {
  lcd.setCursor(cursorPosition * 3, 1 + editModeStep);
  if (millis() % 1000 < 500) {
    lcd.print("  ");
  } else {
    if (cursorPosition == 0) lcd.print(editHour < 10 ? "0" : "");
    if (cursorPosition == 1) lcd.print(editMinute < 10 ? "0" : "");
    if (cursorPosition == 2) lcd.print(editSecond < 10 ? "0" : "");
    if (cursorPosition == 3) lcd.print(editDay < 10 ? "0" : "");
    if (cursorPosition == 4) lcd.print(editMonth < 10 ? "0" : "");
    if (cursorPosition == 5) lcd.print(editYear);
  }
}

void saveDateTime() {
  rtc.adjust(DateTime(editYear, editMonth, editDay, editHour, editMinute, editSecond));
  lcd.setCursor(0, 1 + editModeStep);
  if (editModeStep == 0) {
    lcd.print(editHour < 10 ? "0" : "");
    lcd.print(editHour);
    lcd.print(":");
    lcd.print(editMinute < 10 ? "0" : "");
    lcd.print(editMinute);
    lcd.print(":");
    lcd.print(editSecond < 10 ? "0" : "");
    lcd.print(editSecond);
  } else {
    lcd.print(editDay < 10 ? "0" : "");
    lcd.print(editDay);
    lcd.print(" ");
    lcd.print(editMonth < 10 ? "0" : "");
    lcd.print(editMonth);
    lcd.print(", ");
    lcd.print(editYear);
  }
}

void editDateTime() {
  lcd.setCursor(0, 1 + editModeStep);
  lcd.print(editHour < 10 ? "0" : "");
  lcd.print(editHour);
  lcd.print(":");
  lcd.print(editMinute < 10 ? "0" : "");
  lcd.print(editMinute);
  lcd.print(":");
  lcd.print(editSecond < 10 ? "0" : "");
  lcd.print(editSecond);
  lcd.setCursor(0, 3);
  lcd.print(editDay < 10 ? "0" : "");
  lcd.print(editDay);
  lcd.print(" ");
  lcd.print(editMonth < 10 ? "0" : "");
  lcd.print(editMonth);
  lcd.print(", ");
  lcd.print(editYear);
  blinkCursor();
}