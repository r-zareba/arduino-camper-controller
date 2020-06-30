#include <Arduino.h>

#include "Arduino-LiquidCrystal-I2C-library-master/LiquidCrystal_I2C.h"
#include "Keypad/Keypad.h"
#include "OneWire/OneWire.h"
#include "DallasTemperature/DallasTemperature.h"

#include "Button/Button.h"
#include "Encoder/Encoder.h"


#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3

#define DOOR_SENSOR_1_PIN 6
#define DOOR_SENSOR_2_PIN 5
#define DOOR_SENSOR_3_PIN 4

#define ARMED_BLINK_LED_PIN 1
#define ALARM_RELAY_PIN 0

const uint8_t sensor1pin = A0;
const uint8_t sensor2pin = A1;
const uint8_t sensor3pin = A2;
const uint8_t sensor4pin = A3;
const uint8_t sensor5pin = A4;

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

char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
        {'1','2','3'},
        {'4','5','6'},
        {'7','8','9'},
        {'*','0','#'}
};

const char armingKey = '#';
const char pin[] {'1', '2', '3', '4'};
byte pinPosition;

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);
//Button enterButton(MENU_ENTER_BUTTON_PIN, LOW);
//Button backButton(MENU_BACK_BUTTON_PIN, LOW);
Button doorSensor1(DOOR_SENSOR_1_PIN, LOW);
Button doorSensor2(DOOR_SENSOR_2_PIN, LOW);
Button doorSensor3(DOOR_SENSOR_3_PIN, LOW);
Encoder encoder(ENCODER_PIN_A, ENCODER_PIN_B, true);

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
const unsigned long TIME_TO_UNLOCK = 5000; // 5s
const unsigned long TEMP_UPDATE_TIME = 15000; // 15s since its costly operation

unsigned long blinkTime;
unsigned long alarmTime;
unsigned long countTime;
unsigned long temperatureReadTime;

volatile bool rotating;
volatile bool posChanged;
uint8_t encoderPos;

bool isCountingDown;
bool passwordVerified;
byte nAlarmRetries;

float temperature;

void blinkPin(byte pinNum, unsigned int time);
void countDown();
void stopBlinking();
void turnOnAlarm();
void turnOffAlarm();
bool checkPassword(char key);
void printParams(const String &param1, float value1, const String &param2, float value2);

void encodePinA();
void encodePinB();


void setup() {
    Serial.begin(9600);

//    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(ARMED_BLINK_LED_PIN, OUTPUT);
    pinMode(ALARM_RELAY_PIN, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), encodePinA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), encodePinB, CHANGE);

    controllerState = NORMAL;
    pinPosition = 1;
    isCountingDown = false;
    passwordVerified = false;

    lcd.begin();
    lcd.clear();
    lcd.home();

    encoder.resetPos();
    sensors.begin();
}

void loop() {

    currentTime = millis();
    rotating = true;
    encoderPos = encoder.getCurrentPos();

    if (currentTime - temperatureReadTime >= TEMP_UPDATE_TIME) {
        sensors.requestTemperatures();
        temperature = sensors.getTempCByIndex(0);
        temperatureReadTime = currentTime;
    }

    if (posChanged){
        lcd.clear();
        lcd.backlight();
        posChanged = false;
    }

    switch (encoderPos) {
        case 0:
            printParams("Temp", temperature, "Humidty", 67.8);
            break;
        case 1:
            printParams("Voltage", 11.9, "Current", 2.67);
            break;
        case 2:
            printParams("VOLT", 12.9, "Currento", 2.27);
            break;
        default:
            lcd.clear();
            break;
    }

    char key;

    switch(controllerState) {
        case NORMAL:
            nAlarmRetries = 0;
            key = keypad.getKey();
            if (key)
                Serial.println(key);

            // Stop counting down by pressing any key
            if (isCountingDown) {
                countDown();
                if (key) {
                    isCountingDown = false;
                    break;
                }
            }
            if (key == armingKey) {
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
            break;

        case UNLOCKING:
            blinkPin(ARMED_BLINK_LED_PIN, DISARMING_BLINK_TIME);
            key = keypad.getKey();
            if (key) {
                passwordVerified = checkPassword(key);
                if (passwordVerified) {
                    stopBlinking();
                    controllerState = NORMAL;
                    pinPosition = 1;
                }
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

            key = keypad.getKey();
            if (key) {
                passwordVerified = checkPassword(key);
                if (passwordVerified) {
                    turnOffAlarm();
                    stopBlinking();
                    controllerState = NORMAL;
                    pinPosition = 1;
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

bool checkPassword(char key) {
    if (pinPosition == 1 && key == pin[0]) {
        pinPosition++;
    } else if (pinPosition == 2 && key == pin[1]) {
        pinPosition++;
    } else if (pinPosition == 3 && key == pin[2]) {
        pinPosition++;
    } else if (pinPosition == 4 && key == pin[3]) {
        return true;
    }
    return false;
}

void printParams(const String &param1, float value1, const String &param2, float value2) {
    lcd.setCursor(0, 0);
    lcd.print(param1);
    lcd.setCursor(12, 0);
    lcd.print(value1);

    lcd.setCursor(0, 1);
    lcd.print(param2);
    lcd.setCursor(12, 1);
    lcd.print(value2);
}


/* Interrupts encoder functions */
void encodePinA() {
    if (rotating)
        delay(Encoder::encoderDelay);
    encoder.encodeA();
    rotating = false;
    posChanged = true;
}

void encodePinB() {
    if (rotating)
        delay(Encoder::encoderDelay);
    encoder.encodeB();
    rotating = false;
    posChanged = true;
}
