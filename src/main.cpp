#include <Arduino.h>

#include "Arduino-LiquidCrystal-I2C-library-master/LiquidCrystal_I2C.h"
#include "Keypad/Keypad.h"
#include "OneWire/OneWire.h"
#include "DallasTemperature/DallasTemperature.h"

#include "Button/Button.h"


#define DOOR_SENSOR_1_PIN 5
#define DOOR_SENSOR_2_PIN 4
#define DOOR_SENSOR_3_PIN 3
#define MENU_BUTTON_PIN 6

#define ARMED_BLINK_LED_PIN 2
#define ALARM_RELAY_PIN 1

#define BATTERY_1_VOLTMETER_ANALOG_PIN A0
#define BATTERY_2_VOLTMETER_ANALOG_PIN A1
#define BATTERY_2_AMMETER_ANALOG_PIN A2

#define SECOND_BATTERY_RELAY_PIN 0

const float SECOND_BATTERY_CHARGE_THRESHOLD = 4.51;
const unsigned int SECOND_BATTERY_CHARGE_AFTER = 5000;

const unsigned int ARMED_BLINK_TIME = 500;
const unsigned int DISARMING_BLINK_TIME = 150;
const unsigned int ALARM_DURATION = 5000;
const unsigned int ALARM_COUNTDOWN = 5000;
const byte ALARM_RETRIES = 3;

// Keyboard configuration
const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 3;

byte rowPins[KEYPAD_ROWS] = {7, 8, 9, 10};
byte colPins[KEYPAD_COLS] = {11, 12, 13};

char keyMap[KEYPAD_ROWS][KEYPAD_COLS] = {
        {'1','2','3'},
        {'4','5','6'},
        {'7','8','9'},
        {'*','0','#'}
};

const char ARMING_KEY = '#';
const char RESET_PIN_KEY = '*';
const char PIN_PASSPHRASE[] {'1', '2', '3', '4'};
byte pinPosition;

Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);
Button menuButton(MENU_BUTTON_PIN, LOW);
Button doorSensor1(DOOR_SENSOR_1_PIN, LOW);
Button doorSensor2(DOOR_SENSOR_2_PIN, LOW);
Button doorSensor3(DOOR_SENSOR_3_PIN, LOW);

LiquidCrystal_I2C lcd(0x27, 16, 2); // Change the 0x27

OneWire oneWire(A3);
DallasTemperature sensors(&oneWire);

enum {
    NORMAL = 1,
    ARMED,
    UNLOCKING,
    ALARM
} controllerState;

unsigned long currentTime;
unsigned long unlockTime;
const unsigned long TIME_TO_UNLOCK = 5000;
const unsigned long TEMP_UPDATE_TIME = 15000; // 15s since its costly operation
const unsigned long LCD_BACKLIGHT_TIME = 15000;
const unsigned long ANALOG_READ_TIME = 100;

unsigned long blinkTime;
unsigned long alarmTime;
unsigned long countTime;
unsigned long temperatureReadTime;
unsigned long lcdBacklightTime;
unsigned long analogReadTime;
unsigned long secondBatteryChargeTime;

byte menuPosition;
char insertedKey;

bool isCountingDown;
bool passwordVerified;
byte nAlarmRetries;

bool screenTurnedOff;

double temperature;
double batteryVoltage1;
double batteryVoltage2;
double batteryCurrent2;

bool isCharging;
bool chargeCountdown;


void blinkPin(byte pinNum, unsigned int time);
void countDown();
void stopBlinking();
void disarmAlarm();
void turnOnAlarm();
void turnOffAlarm();
bool checkPassword(char insertedChar);
void printParam(const String &param, double value, byte row);
void printParams(const String &param1, double value1, const String &param2, double value2);
void keepInRange(byte &value, int min, int max);


void setup() {
    pinMode(ARMED_BLINK_LED_PIN, OUTPUT);
    pinMode(ALARM_RELAY_PIN, OUTPUT);
    pinMode(SECOND_BATTERY_RELAY_PIN, OUTPUT);

    controllerState = NORMAL;
    pinPosition = 1;
    isCountingDown = false;
    passwordVerified = false;

    lcd.begin();
    lcd.clear();
    lcd.home();
    screenTurnedOff = false;

    isCharging = false;
    chargeCountdown = false;
    sensors.begin();

}

void loop() {

    currentTime = millis();
    insertedKey = keypad.getKey();

    if (insertedKey == RESET_PIN_KEY)
        pinPosition = 1;

    if (menuButton.beenClicked()) {
        lcd.clear();
        lcd.backlight();
        lcdBacklightTime = currentTime;
        if (screenTurnedOff) {
            screenTurnedOff = false;
        } else {
            menuPosition++;
            keepInRange(menuPosition, 0, 2);
        }
    }

    if (currentTime - temperatureReadTime >= TEMP_UPDATE_TIME) {
        sensors.requestTemperatures();
        temperature = sensors.getTempCByIndex(0);
        temperatureReadTime = currentTime;
    }

    if (currentTime - lcdBacklightTime >= LCD_BACKLIGHT_TIME) {
        lcd.noBacklight();
        screenTurnedOff = true;
    }

    if (currentTime - analogReadTime >= ANALOG_READ_TIME) {
        batteryVoltage1 = analogRead(BATTERY_1_VOLTMETER_ANALOG_PIN) * (5.0 / 1024.0);
        batteryVoltage2 = analogRead(BATTERY_2_VOLTMETER_ANALOG_PIN) * (5.0 / 1024.0);
        batteryCurrent2 = analogRead(BATTERY_2_AMMETER_ANALOG_PIN) * (5.0 / 1024.0);
        analogReadTime = currentTime;
    }

    if (batteryVoltage1 > SECOND_BATTERY_CHARGE_THRESHOLD && !isCharging) {
        secondBatteryChargeTime = currentTime;
        isCharging = true;
    }

    if (currentTime - secondBatteryChargeTime >= SECOND_BATTERY_CHARGE_AFTER && isCharging)
        digitalWrite(SECOND_BATTERY_RELAY_PIN, HIGH);

    if (batteryVoltage1 < SECOND_BATTERY_CHARGE_THRESHOLD) {
        digitalWrite(SECOND_BATTERY_RELAY_PIN, LOW);
        isCharging = false;
    }


    switch (menuPosition) {
        case 0:
            printParams("Temp [C]", temperature, "Humidity", 62.7);
            break;
        case 1:
            printParam("BAT 1 [U]", batteryVoltage1, 0);
            break;
        case 2:
            printParams("BAT 2 [U]", batteryVoltage2, "BAT 2 [I]", batteryCurrent2);
            break;
        default:
            lcd.clear();
            break;
    }

    switch(controllerState) {
        case NORMAL:
            nAlarmRetries = 0;

            // Stop counting down by pressing any key
            if (isCountingDown) {
                countDown();
                if (insertedKey) {
                    isCountingDown = false;
                    break;
                }
            }
            if (insertedKey == ARMING_KEY) {
                isCountingDown = true;
                countTime = currentTime;
            }
            break;

        case ARMED:
            blinkPin(ARMED_BLINK_LED_PIN, ARMED_BLINK_TIME);
            if (doorSensor2.beenClicked() || doorSensor2.isPressed() ||
            doorSensor3.beenClicked() || doorSensor3.isPressed()) {
                alarmTime = currentTime;
                stopBlinking();
                controllerState = ALARM;
            } else if (doorSensor1.beenClicked()) {
                unlockTime = currentTime;
                controllerState = UNLOCKING;
            }

            if (insertedKey) {
                passwordVerified = checkPassword(insertedKey);
                if (passwordVerified)
                    disarmAlarm();
            }
            break;

        case UNLOCKING:
            blinkPin(ARMED_BLINK_LED_PIN, DISARMING_BLINK_TIME);
            if (insertedKey) {
                passwordVerified = checkPassword(insertedKey);
                if (passwordVerified)
                    disarmAlarm();
            }

            if (currentTime - unlockTime >= TIME_TO_UNLOCK) {
                alarmTime = currentTime;
                controllerState = ALARM;
            }
            break;

        case ALARM:
            if (nAlarmRetries < ALARM_RETRIES) {
                stopBlinking();
                turnOnAlarm();
            } else
                blinkPin(ARMED_BLINK_LED_PIN, DISARMING_BLINK_TIME);

            if (insertedKey) {
                passwordVerified = checkPassword(insertedKey);
                if (passwordVerified) {
                    turnOffAlarm();
                    disarmAlarm();

                }
            }
            break;
    }
}


void countDown() {
    if (currentTime - countTime >= ALARM_COUNTDOWN) {
        controllerState = ARMED;
        isCountingDown = false;
    }
}

void blinkPin(byte pinNum, unsigned int time) {
    bool ledState = digitalRead(pinNum);
    if (currentTime - blinkTime >= time) {
        ledState = !ledState;
        digitalWrite(pinNum, ledState);
        blinkTime = currentTime;
    }
}

void stopBlinking() {
    digitalWrite(ARMED_BLINK_LED_PIN, LOW);
}

void disarmAlarm() {
    stopBlinking();
    controllerState = NORMAL;
    pinPosition = 1;
}

void turnOnAlarm() {
    digitalWrite(ALARM_RELAY_PIN, HIGH);
    if (currentTime - alarmTime >= ALARM_DURATION) {
        turnOffAlarm();
        nAlarmRetries++;
        controllerState = ARMED;
    }
}

void turnOffAlarm() {
    digitalWrite(ALARM_RELAY_PIN, LOW);
}

bool checkPassword(char insertedChar) {
    if (pinPosition == 1 && insertedChar == PIN_PASSPHRASE[0]) {
        pinPosition++;
    } else if (pinPosition == 2 && insertedChar == PIN_PASSPHRASE[1]) {
        pinPosition++;
    } else if (pinPosition == 3 && insertedChar == PIN_PASSPHRASE[2]) {
        pinPosition++;
    } else if (pinPosition == 4 && insertedChar == PIN_PASSPHRASE[3]) {
        return true;
    }
    return false;
}

void printParam(const String &param, double value, byte row) {
    lcd.setCursor(0, row);
    lcd.print(param);
    lcd.setCursor(12, row);
    lcd.print(value);
}

void printParams(const String &param1, double value1, const String &param2, double value2) {
    printParam(param1, value1, 0);
    printParam(param2, value2, 1);
}

void keepInRange(byte &value, int min, int max) {
    if (value > max)
        value = min;
    if (value < min)
        value = max;
}
