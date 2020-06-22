//
// Created by rafal on 22.06.2020.
//

#include "Button.h"


unsigned long Button::lastTimeClicked = 0;
const uint8_t Button::buttonDelay = 20;


Button::Button(uint8_t pin, bool idleState)
: pin(pin), idleState(idleState) {
    pinMode(pin, INPUT);
}


bool Button::beenClicked() {
    /* Implementation of checking if button has been clicked
       Reduces debouncing effect */
    currentState = digitalRead(pin);

    if (currentState != lastButtonState)
        lastTimeClicked = millis();

    if ((millis() - lastTimeClicked) > buttonDelay) {
        if (currentState != buttonState) {
            buttonState = currentState;

            if (buttonState != idleState) {
                lastButtonState = currentState;
                return true;
            }
        }
    }

    lastButtonState = currentState;
    return false;
}

