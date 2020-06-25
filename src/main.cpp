#include <Arduino.h>
#include <Wire.h>

//#include <LiquidCrystal_I2C.h>

#include "Button/Button.h"
#include "Encoder/Encoder.h"
#include "Keypad/Keypad.h"

#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3

#define MENU_ENTER_BUTTON_PIN 46
#define MENU_BACK_BUTTON_PIN 44
#define DOOR_SENSOR_1_PIN 48
#define DOOR_SENSOR_2_PIN 50
#define DOOR_SENSOR_3_PIN 52

#define GREEN_LED_PIN 8
#define ALARM_BLINK_LED_PIN 9
#define ALARM_RELAY_PIN 10

const uint8_t sensor1pin = A0;
const uint8_t sensor2pin = A1;
const uint8_t sensor3pin = A2;
const uint8_t sensor4pin = A3;
const uint8_t sensor5pin = A4;

const int LED_BLINK_TIME = 500;
const unsigned long ALARM_DURATION = 10000; // 10s
const unsigned long ALARM_COUNTDOWN = 10000; // 10s

// Keyboard configuration
const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 3;

const byte rowPins[KEYPAD_ROWS] = {43, 41, 39, 37};
const byte colPins[KEYPAD_COLS] = {35, 33, 31};

char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
        {'1','2','3'},
        {'4','5','6'},
        {'7','8','9'},
        {'*','0','#'}
};

char pin[] {'1', '2', '3', '4'};
int pinPosition = 1;

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);
Button enterButton(MENU_ENTER_BUTTON_PIN, LOW);
Button backButton(MENU_BACK_BUTTON_PIN, LOW);
Button doorSensor1(DOOR_SENSOR_1_PIN, LOW);
Button doorSensor2(DOOR_SENSOR_2_PIN, LOW);
Button doorSensor3(DOOR_SENSOR_3_PIN, LOW);
Encoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);

enum {
    NORMAL = 1,
    ARMED,
    UNLOCKING,
    ALARM
} controllerState;

unsigned long currentTime;
unsigned long unlockTime;
const unsigned long TIME_TO_UNLOCK = 5000; // 5s

unsigned long blinkTime;
unsigned long alarmTime;
unsigned long countTime;

volatile bool rotating;
volatile bool posChanged;

bool isCountingDown = false;
bool countedDown = false;

void countDown(unsigned long time);
void blinkLed(byte ledPin, unsigned long time);
void stopBlinking(byte ledPin);
void turnOnAlarm(byte relayPin, unsigned long time);
void turnOffAlarm(byte relayPin);

void encodePinA();
void encodePinB();


void setup() {
    Serial.begin(9600);

    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(ALARM_BLINK_LED_PIN, OUTPUT);
    pinMode(ALARM_RELAY_PIN, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), encodePinA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), encodePinB, CHANGE);

    controllerState = NORMAL;
}

void loop() {

    currentTime = millis();
    rotating = true;

    char key;

    switch(controllerState) {
        case NORMAL:
            key = keypad.getKey();

            // Stop counting down by pressing any key
            if (isCountingDown) {
                countDown(ALARM_COUNTDOWN);
                if (key) {
                    isCountingDown = false;
                    break;
                }

            }

            if (key == '#') {
                isCountingDown = true;
                countTime = currentTime;
            }
            break;

        case ARMED:
            blinkLed(ALARM_BLINK_LED_PIN, LED_BLINK_TIME);
            if (doorSensor2.beenClicked() || doorSensor3.beenClicked()) {
                alarmTime = currentTime;
                controllerState = ALARM;
            } else if (doorSensor1.beenClicked()) {
                unlockTime = currentTime;
                controllerState = UNLOCKING;
            }
            break;

        case UNLOCKING:
            blinkLed(ALARM_BLINK_LED_PIN, 200);
            key = keypad.getKey();
            if (key) {
                if (pinPosition == 1 && key == pin[0]) {
                    pinPosition++;
                } else if (pinPosition == 2 && key == pin[1]) {
                    pinPosition++;
                } else if (pinPosition == 3 && key == pin[2]) {
                    pinPosition++;
                } else if (pinPosition == 4 && key == pin[3]) {
                    stopBlinking(ALARM_BLINK_LED_PIN);
                    controllerState = NORMAL;
                    pinPosition = 1;
                } else {
                    pinPosition = 1;
                }
            }

            if (currentTime - unlockTime >= TIME_TO_UNLOCK) {
                alarmTime = currentTime;
                controllerState = ALARM;
            }
            break;

        case ALARM:
            stopBlinking(ALARM_BLINK_LED_PIN);
            turnOnAlarm(ALARM_RELAY_PIN, ALARM_DURATION);
            break;
    }

}

//    if (enterButton.beenClicked()) {
//        ledState = !ledState;
//        digitalWrite(greenLedPin, ledState);
//    }
//
////    if (currentTime - printTime >= 100) {
////        Serial.println(encoder.getCurrentPos());
////        printTime = currentTime;
////    }
//
//    char key = keypad.getKey();
//    if (key)
//        Serial.println(key);

void countDown(unsigned long time) {
    if (currentTime - countTime >= time) {
        controllerState = ARMED;
        isCountingDown = false;
    }
}

void blinkLed(byte ledPin, unsigned long time) {
    bool ledState = digitalRead(ledPin);

    if (currentTime - blinkTime >= time) {
        ledState = !ledState;
        digitalWrite(ledPin, ledState);
        blinkTime = currentTime;
    }
}

void stopBlinking(byte ledPin) {
    digitalWrite(ledPin, LOW);
}

void turnOnAlarm(byte relayPin, unsigned long time) {
    digitalWrite(relayPin, HIGH);
    if (currentTime - alarmTime >= time) {
        turnOffAlarm(relayPin);
        controllerState = ARMED;
    }
}

void turnOffAlarm(byte relayPin) {
    digitalWrite(relayPin, LOW);
}


/* Interrupts functions */
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
