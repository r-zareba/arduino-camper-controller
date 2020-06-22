#include <Arduino.h>
#include <Wire.h>
//#include <LiquidCrystal_I2C.h>

#include "Button/Button.h"
#include "Encoder/Encoder.h"

#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3

#define ENTER_BUTTON_PIN 51
#define BACK_BUTTON_PIN 52

const uint8_t greenLedPin = 8;
const uint8_t yellowLedPin = 9;
const uint8_t redLedPin = 10;

const uint8_t sensor1pin = A0;
const uint8_t sensor2pin = A1;
const uint8_t sensor3pin = A2;
const uint8_t sensor4pin = A3;
const uint8_t sensor5pin = A4;

Button enterButton(ENTER_BUTTON_PIN, LOW);
Button backButton(BACK_BUTTON_PIN, LOW);

Encoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);




unsigned long currentTime;
unsigned long printTime;

//void sendSensorData(int &temperature);
//void lightLed(char &led, uint8_t &state);

int temperature;
bool ledState = LOW;

volatile bool rotating;
volatile bool posChanged;

void encodePinA();
void encodePinB();


void setup() {

    Serial.begin(9600);

    pinMode(greenLedPin, OUTPUT);
    pinMode(yellowLedPin, OUTPUT);
    pinMode(redLedPin, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), encodePinA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), encodePinB, CHANGE);

}

void loop() {

    currentTime = millis();

    if (enterButton.beenClicked()) {
        ledState = !ledState;
        digitalWrite(greenLedPin, ledState);
    }

    rotating = true;

    if (currentTime - printTime >= 100) {
        Serial.println(encoder.getCurrentPos());
        printTime = currentTime;
    }
}

void encodePinA() {
    if (rotating)
        delay(Encoder::encoderDelay);
    encoder.encodeA();
    rotating=false;
    posChanged = true;
}


void encodePinB() {
    if (rotating)
        delay(Encoder::encoderDelay);
    encoder.encodeB();
    rotating=false;
    posChanged = true;
}



