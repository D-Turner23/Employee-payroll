// Refactored content of the F429540_.ino file

#include <SomeLibrary.h> // Include necessary libraries

// Constants
const int LED_PIN = 13; // Example of a constant definition

// Helper Functions
void setupLED() {
    pinMode(LED_PIN, OUTPUT);
}

void turnOnLED() {
    digitalWrite(LED_PIN, HIGH);
}

void turnOffLED() {
    digitalWrite(LED_PIN, LOW);
}

// Setup function
void setup() {
    setupLED();
}

// Loop function
void loop() {
    turnOnLED();
    delay(1000); // Leave LED on for 1 second
    turnOffLED();
    delay(1000); // Leave LED off for 1 second
}