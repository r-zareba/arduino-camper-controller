//
// Created by rafal on 22.06.2020.
//

#ifndef HOME_CONTROLLER_ENCODER_H
#define HOME_CONTROLLER_ENCODER_H

#include <HID.h>

class Encoder {

public:

    static const uint8_t encoderDelay;

    Encoder(uint8_t pinA, uint8_t pinB);
    Encoder(uint8_t pinA, uint8_t pinB, int min, int max);

    void encodeA();
    void encodeB();
    void setCurrentPos(uint8_t pos);
    uint8_t getCurrentPos() const;
    void resetPos();
    void setRange(uint8_t min, uint8_t max);
    void keepInRange();
    bool isRotatingLeft() const;
    bool isRotatingRight() const;
    void resetRotating();

private:
    const uint8_t pinA;
    const uint8_t pinB;
    uint8_t minRange;
    uint8_t maxRange;
    int8_t currentPos = minRange;
    bool encoderAset = LOW;
    bool encoderBset = LOW;
    bool rotatingRight = false;
    bool rotatingLeft = false;




};


#endif //HOME_CONTROLLER_ENCODER_H
