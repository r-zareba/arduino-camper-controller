//
// Created by rafal on 22.06.2020.
//

#include "Encoder.h"

const uint8_t Encoder::encoderDelay = 10;


Encoder::Encoder(uint8_t pinA, uint8_t pinB, bool overflow)
        : pinA(pinA), pinB(pinB), minRange(0), maxRange(2), overflow(overflow) {
    pinMode(pinA, INPUT);
    pinMode(pinB, INPUT);
}

Encoder::Encoder(uint8_t pinA, uint8_t pinB, bool overflow, int min, int max)
        : pinA(pinA), pinB(pinB), minRange(min), maxRange(max), overflow(overflow) {
    pinMode(pinA, INPUT);
    pinMode(pinB, INPUT);
}

void Encoder::keepInRange() {
    if (overflow) {
        if (currentPos > maxRange)
            currentPos = minRange;
        if (currentPos < minRange)
            currentPos = maxRange;
    } else {
        if (currentPos >= maxRange)
            currentPos = maxRange;
        if (currentPos <= minRange)
            currentPos = minRange;
    }
}

void Encoder::encodeA() {
    bool pinState = digitalRead(pinA);

    if (pinState != encoderAset) {
        encoderAset = !encoderAset;

        if (encoderAset && !encoderBset) {
            currentPos++;
            rotatingLeft = false;
            rotatingRight = true;
        }
    }
    keepInRange();
}

void Encoder::encodeB() {
    bool pinState = digitalRead(pinB);

    if (pinState != encoderBset) {
        encoderBset = !encoderBset;

        if (encoderBset && !encoderAset){
            currentPos--;
            rotatingRight = false;
            rotatingLeft = true;
        }
    }
    keepInRange();
}

uint8_t Encoder::getCurrentPos() const {
    return currentPos;
}

void Encoder::setCurrentPos(uint8_t pos) {
    currentPos = pos;
}

void Encoder::resetPos() {
    setCurrentPos(minRange);
}

void Encoder::setRange(uint8_t min, uint8_t max) {
    minRange = min;
    maxRange = max;
    keepInRange();
}

bool Encoder::isRotatingLeft() const {
    return rotatingLeft;
}

bool Encoder::isRotatingRight() const {
    return rotatingRight;
}

void Encoder::resetRotating() {
    rotatingLeft = false;
    rotatingRight = false;
}
