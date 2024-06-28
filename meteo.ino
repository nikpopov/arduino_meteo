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

#define BTN_EDIT_PIN 3   // Define EDIT buttin pin
#define BTN_SAVE_PIN 4    // Define SAVE buttin pin
#define BTN_UP_PIN 5     // Define UP buttin pin
#define BTN_DOWN_PIN 6   // Define DOWN buttin pin

#define DEBOUNCE_DELAY 50   // Define debounce time
#define LONG_PRESS_DURATION 1000 // Define long press time setting for EDIT mode enter

DHT dht(DHTPIN, DHTTYPE);   // Define DHT sensor
Adafruit_BMP085 bmp;        // Define BMP sensor
RTC_DS1307 rtc;             // Define DS1307RTC
LiquidCrystal_I2C lcd(0x27, 20, 4);;  // Set the LCD address to 0x27 for a 20 chars and 4 line display

unsigned long lastDebounceTime = 0; // Time the button was last toggled
unsigned long lastEditDebounceTime = 0; // Time the button was last toggled
unsigned long debounceDelay = 30;   // Debounce time, increase if needed

unsigned long lastButtonPressTime = 0;

unsigned long pressStartTime;
const unsigned long longPressDuration = 1000; // Duration for a long press (in ms)

bool btnEditState = HIGH;
bool btnEditPrevState = HIGH;
bool btnUpState;
bool btnUpFlag = false;
bool btnDownState;
bool btnDownFlag = false;
bool btnSaveState;
bool btnSaveFlag = false;

bool isTimerEdit = false;
bool timerEditPrevious = false;
bool timerEditDisplayAccess = false;

bool editing = false;

int cursorPosition = 0; // 0 for day, 1 for month, 2 for year
int editModeStep = 0; // 0 for time, 1 for date
bool backlightOn = true;
unsigned long backlightTimer = 0;

int btnStatus = HIGH;
bool btnStatusFlag = false;
int lastButtonStatus = HIGH;
unsigned long debounceTime = 0;
bool isEdit = false;

bool editTimeMode = true;
bool editDateMode = false;
bool exitEdit = false;
bool editTimerMenuBlocked = false;

// Initial time and date settings
int editHour = 0, editMinute = 0, editSecond = 0;
int editDay = 1, editMonth = 1, editYear = 2020;

// Initial sensor's data set up
float temperature = 0.00;
float temperaturePrevious = 0.00;
float humidity = 0.00;
float humidityPrevious = 0.00;
float pressure = 0.00;
float pressurePrevious = 0.00;

// Initial time and date setting
int hour = 0;
int minute = 0;
int second = 0;
int day = 0;
int month = 0;
int year = 0;

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
    rtc.adjust(DateTime(2022, 1, 1, 0, 0, 0));
  }

  lcd.init();
  lcd.backlight();
  updateDisplay();

  pinMode(BTN_EDIT_PIN, INPUT_PULLUP); // Initialize EDIT button and pull_up resistor
  pinMode(BTN_SAVE_PIN, INPUT_PULLUP); // Initialize SAVE button and pull_up resistor
  pinMode(BTN_UP_PIN, INPUT_PULLUP);   // Initialize UP button and pull_up resistor
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP); // Initialize DOWN button and pull_up resistor
}

// void handleButtons() {
//   int btnSaveReading = digitalRead(BTN_SAVE_PIN);
//   if (btnSaveReading != btnStatus) {
//     debounceTime = millis();
//   }
//   if ((millis() - debounceTime) > DEBOUNCE_DELAY) {
//     if (!isTimerEdit) {
//       timerEditDisplay();
//       isTimerEdit = true;
//     } else {

//     }

//   }
//   btnStatus = btnSaveReading;

//   int reading = digitalRead(BTN_EDIT_PIN);
//   if (reading != btnEditPrevState) {
//     lastDebounceTime = millis();
//   }
//   if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
//     if (reading != btnEditState) {
//       btnEditState = reading;
//       if (btnEditState == LOW) {
//         lastButtonPressTime = millis();
//       } else {
//         if ((millis() - lastButtonPressTime) > LONG_PRESS_DURATION) {
//           if (!editing) {
//             editing = true;
//             cursorPosition = 0;
//             editModeStep = 0;
//             DateTime now = rtc.now();
//             editHour = now.hour();
//             editMinute = now.minute();
//             editSecond = now.second();
//             editDay = now.day();
//             editMonth = now.month();
//             editYear = now.year();
//             lcd.clear();
//             lcd.setCursor(0, 0);
//             lcd.print("Edit Mode: Time");
//           } else {
//             saveDateTime();
//             editing = false;
//             lcd.clear();
//             lcd.setCursor(0, 0);
//             lcd.print("Temp:  Humidity:");
//             lcd.setCursor(0, 2);
//             lcd.print("Pressure:");
//           }
//         } else {
//           if (editing) {
//             editModeStep = (editModeStep + 1) % 2;
//             if (editModeStep == 0) {
//               cursorPosition = 0;
//               lcd.setCursor(0, 0);
//               lcd.print("Edit Mode: Time");
//             } else {
//               cursorPosition = 3;
//               lcd.setCursor(0, 0);
//               lcd.print("Edit Mode: Date");
//             }
//           }
//         }
//       }
//     }
//   }
//   btnEditPrevState = reading;

//   if (editing) {
//     if (digitalRead(BTN_UP_PIN) == LOW) {
//       incrementValue();
//       delay(200);
//     }
//     if (digitalRead(BTN_DOWN_PIN) == LOW) {
//       decrementValue();
//       delay(200);
//     }
//     if (digitalRead(BTN_SAVE_PIN) == LOW) {
//       saveDateTime();
//       cursorPosition++;
//       if (editModeStep == 0 && cursorPosition > 2) cursorPosition = 0;
//       if (editModeStep == 1 && cursorPosition > 5) cursorPosition = 3;
//       delay(200);
//     }
//   }
// }


void loop() {
  //handleButtons();    // Handle whether EDIT mode is active
  handleBtns();
  if (isTimerEdit) {
    if (timerEditDisplayAccess) {
      timerEditDisplay();
    }
  } else {
    displaySensorLabels();
    displaySensorData();
    displayDateTime();
  }
  
  handleBacklight();  // Handle LCD backlight for energy saving
  // delay(100);
}

void handleBtns() {
  // Handle Edit button
  bool readEdit = digitalRead(BTN_EDIT_PIN); // Read the state of the button
  
  // Check if the button state has changed
  if (readEdit != btnEditPrevState) {
    lastEditDebounceTime = millis(); // Reset the debouncing timer
  }

  if ((millis() - lastEditDebounceTime) > debounceDelay) {
    // If the button state has changed
    if (readEdit != btnEditState) {
      btnEditState = readEdit;
      
      // If the button is pressed
      if (btnEditState == LOW) {
        pressStartTime = millis(); // Record the time when the button is pressed
      } else {
        unsigned long pressDuration = millis() - pressStartTime;

        if (pressDuration < longPressDuration) {
          Serial.println("Tick");
          if (isTimerEdit) {
            timerEditPrevious = isTimerEdit;
            isTimerEdit = false;
            timerEditDisplayAccess = false;
            editTimeMode = true;
            editDateMode = false;
            exitEdit = false;
            updateDisplay();
          }
        } else {
          Serial.println("Press");
          isTimerEdit = true;
          timerEditDisplayAccess = true;
        }
      }
    }
  }

  btnEditPrevState = readEdit;
  
  // Handle Up button
  btnUpState = !digitalRead(BTN_UP_PIN);

  if (btnUpState == HIGH && btnUpFlag == false) {
    btnUpFlag = true;
    // Serial.println("BTN_UP Pressed");
  }
  if (btnUpState == LOW && btnUpFlag == true) {
    btnUpFlag = false;
    // Serial.println("BTN_UP Released");
    if (isTimerEdit) {
      timerEditDisplayAccess = true;
      editTimerMenuBlocked = false;
      if (editDateMode) {
        editTimeMode = true;
        editDateMode = false;
        exitEdit = false;
      }
      if (exitEdit) {
        editTimeMode = false;
        editDateMode = true;
        exitEdit = false;
      }
    }
  }

  // Handle Down button
  btnDownState = !digitalRead(BTN_DOWN_PIN);

  if (btnDownState == HIGH && btnDownFlag == false) {
    btnDownFlag = true;
    // Serial.println("BTN_DOWN Pressed");
  }
  if (btnDownState == LOW && btnDownFlag == true) {
    btnDownFlag = false;
    // Serial.println("BTN_DOWN Released");
    if (isTimerEdit) {
      timerEditDisplayAccess = true;
      editTimerMenuBlocked = false;
      if (editDateMode) {
        editTimeMode = false;
        editDateMode = false;
        exitEdit = true;
      }
      if (editTimeMode) {
        editTimeMode = false;
        editDateMode = true;
        exitEdit = false;
      }
    }
  }

  // Handle Save button
  btnSaveState = !digitalRead(BTN_SAVE_PIN);

  if (btnSaveState == HIGH && btnSaveFlag == false) {
    btnSaveFlag = true;
    // Serial.println("BTN_Save Pressed");
  }
  if (btnSaveState == LOW && btnSaveFlag == true) {
    btnSaveFlag = false;
    // Serial.println("BTN_DOWN Released");

    if (isTimerEdit) {
      if (exitEdit) {
        timerEditPrevious = true;
        isTimerEdit = false;
        timerEditDisplayAccess = false;
        editTimeMode = true;
        editDateMode = false;
        exitEdit = false;
        updateDisplay();
      }
    }
  }
}

void updateDisplay() {
  lcd.clear();

  displaySensorLabels();
  displaySensorData();
  displayDateTime();
  timerEditPrevious = false;
}

void displaySensorLabels() {
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

void displaySensorData() {
  // Read temperature and humidity data from DHT sensor
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  // Read pressure data from BMP-180 sensor
  pressure = bmp.readPressure() * 0.00750062;

  if (isnan(temperature) || isnan(humidity) || isnan(pressure)) {
    // lsd.print(""Failed to read from sensors!"");
    Serial.println("Failed to read from sensors!");
    return;
  }
  
  // Display results on LCD
  if (temperature != temperaturePrevious || timerEditPrevious) {
    lcd.setCursor(13, 1);
    lcd.print(temperature);
    temperaturePrevious = temperature;
  }

  if (humidity != humidityPrevious || timerEditPrevious) {
    lcd.setCursor(13, 2);
    lcd.print(humidity);
    humidityPrevious = humidity;
  }
  if (pressure != pressurePrevious || timerEditPrevious) {
    lcd.setCursor(10, 3);
    lcd.print(pressure);
    pressurePrevious = pressure;
  }
}

void displayDateTime() {
  DateTime now = rtc.now();
  int tmpHour = now.hour();
  int tmpMinute = now.minute();
  int tmpSecond = now.second();
  int tmpDay = now.day();
  int tmpMonth = now.month();
  int tmpYear = now.year();

  if (tmpHour != hour || timerEditPrevious) {
    lcd.setCursor(0, 0);
    lcd.print(tmpHour < 10 ? "0" : "");
    lcd.print(tmpHour);
    hour = tmpHour;
  }
  lcd.setCursor(2, 0);
  lcd.print(":");
  if (tmpMinute != minute || timerEditPrevious) {
    lcd.setCursor(3, 0);
    lcd.print(tmpMinute < 10 ? "0" : "");
    lcd.print(tmpMinute);
    minute = tmpMinute;
  }
  lcd.setCursor(5, 0);
  lcd.print(":");
  if(tmpSecond != second || timerEditPrevious) {
    lcd.setCursor(6, 0);
    lcd.print(tmpSecond < 10 ? "0" : "");
    lcd.print(tmpSecond);
    second = tmpSecond;
  }
  
  if (tmpDay != day || timerEditPrevious) {
    lcd.setCursor(10, 0);
    lcd.print(tmpDay < 10 ? "0" : "");
    lcd.print(tmpDay);
    day = tmpDay;
  }
  lcd.setCursor(12, 0);
  lcd.print("-");
  if (tmpMonth != month || timerEditPrevious) {
    lcd.setCursor(13, 0);
    lcd.print(tmpMonth < 10 ? "0" : "");
    lcd.print(tmpMonth);
    month = tmpMonth;
  }
  lcd.setCursor(15, 0);
  lcd.print("-");
  if (tmpYear != year || timerEditPrevious) {
    lcd.setCursor(16, 0);
    lcd.print(tmpYear);
    year = tmpYear;
  }
}

void timerEditDisplay() {
  Serial.println("Edit display");
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("Edit Mode");

  if (editTimeMode) {
    lcd.setCursor(0, 1);
    lcd.print(">");
  }
  if (editDateMode) {
    lcd.setCursor(0, 2);
    lcd.print(">");
  }
  if (exitEdit) {
    lcd.setCursor(0, 3);
    lcd.print(">");
  }

  lcd.setCursor(2, 1);
  lcd.print("Time:");
  lcd.setCursor(10, 1);
  lcd.print(":");
  lcd.setCursor(13, 1);
  lcd.print(":");

  lcd.setCursor(2, 2);
  lcd.print("Date:");
  lcd.setCursor(10, 2);
  lcd.print("-");
  lcd.setCursor(13, 2);
  lcd.print("-");

  lcd.setCursor(2, 3);
  lcd.print("Exit");
  
  lcd.setCursor(8, 1);
  lcd.print(hour < 10 ? "0" : "");
  lcd.print(hour);
  lcd.setCursor(11, 1);
  lcd.print(minute < 10 ? "0" : "");
  lcd.print(minute);
  lcd.setCursor(14, 1);
  lcd.print(second < 10 ? "0" : "");
  lcd.print(second);

  lcd.setCursor(8, 2);
  lcd.print(day < 10 ? "0" : "");
  lcd.print(day);
  lcd.setCursor(11, 2);
  lcd.print(month < 10 ? "0" : "");
  lcd.print(month);
  lcd.setCursor(14, 2);
  lcd.print(year < 10 ? "0" : "");
  lcd.print(year);

  timerEditDisplayAccess = false;
  editTimerMenuBlocked = true;
}

void handleBacklight() {
  if (editing || isTimerEdit || digitalRead(BTN_EDIT_PIN) == LOW || digitalRead(BTN_SAVE_PIN) == LOW || digitalRead(BTN_UP_PIN) == LOW || digitalRead(BTN_DOWN_PIN) == LOW) {
    lcd.backlight();
    backlightOn = true;
    backlightTimer = millis();
  }
  if (backlightOn && millis() - backlightTimer > 30000) {
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

