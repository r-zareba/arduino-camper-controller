//
// Created by rafal on 22.06.2020.
//

#ifndef ARDUINO_CAMPER_CONTROLLER_BUTTON_H
#define ARDUINO_CAMPER_CONTROLLER_BUTTON_H

#include <HID.h>


class Button {
public:
    Button(byte pin, bool idleState);
    bool beenClicked();
    bool isPressed() const;

private:
    static unsigned long lastTimeClicked;
    static const byte buttonDelay;

    const byte pin;
    const bool idleState;
    bool buttonState = idleState;
    bool lastButtonState = idleState;
    bool currentState = idleState;
};

#endif
