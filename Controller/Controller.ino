#define THROTTLE_PIN 11
#define STEERING_RIGT_PIN 10
#define STEERING_LEFT_PIN 9
#define STEERING_POS_PIN A0

#define THROTTLE_RADIO_IN_PIN 7
#define STEERING_RADIO_IN_PIN 8

#define STATUS_PIN 13

#include "Steering.h"
#include "Throttle.h"

Throttle throttle;
Steering steering;

bool radioMode = false;
bool requireCommandStream = true;

unsigned long maxWaitTime = 3000; // 3 seconds
unsigned long lastCommandTime = 0;

void setup() {
  pinMode(THROTTLE_PIN, OUTPUT);
  pinMode(STEERING_RIGT_PIN, OUTPUT);
  pinMode(STEERING_LEFT_PIN, OUTPUT);

  pinMode(THROTTLE_RADIO_IN_PIN, INPUT);
  pinMode(STEERING_RADIO_IN_PIN, INPUT);

  pinMode(STATUS_PIN, OUTPUT);

  // Calibrate with center analog reading, max left/right turn amount
  steering.calibrate(520, 170);
  // Set min/max for throttle speed
  throttle.setMinMaxSpeed(80, 110);

  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect. Needed for Native USB only
  }

  Serial.println("Enter command:");
}

void loop() {
  bool isStopped = steering.isStopped() && throttle.isStopped();

  if (radioMode) {
    handleRadioInput();
  } else if (Serial.available() > 0) {
    lastCommandTime = millis();
    handleSerialInput();
  } else if (
    requireCommandStream
    && millis() - lastCommandTime > maxWaitTime
    && !isStopped
  ) {
    Serial.println("Stopping - no command received");
    stop();
  }

  digitalWrite(STATUS_PIN, isStopped ? LOW : HIGH);

  steering.update();
  throttle.update();

  delay(50);
}

void stop() {
  throttle.stop();
  steering.stop();
}

/**
 * Handle command input over Serial
 *
 * Format: [type] [...params]
 *
 * Types: 0 - set [throttle speed]
 *        1 - set [steering position]
 *        2 - set both [throttle speed] [steering position]
 */
void handleSerialInput() {
  int cmdType = Serial.readStringUntil(' ').toInt();

  if (cmdType == 2) {
    throttle.setSpeed(Serial.parseInt());
    steering.setTargetPos(Serial.parseInt());
  } else if (cmdType == 0) {
    throttle.setSpeed(Serial.parseInt());
  } else if (cmdType == 1) {
    steering.setTargetPos(Serial.parseInt());
  }

  Serial.read();

  Serial.print("Set speed: ");
  Serial.print(throttle.getSpeed());
  Serial.print(", steeringPos: ");
  Serial.println(steering.getOffsetPos());
}

/**
 * Scale radio control input and set values
 */
void handleRadioInput() {
  throttle.setSpeed(map(
    pulseIn(THROTTLE_RADIO_IN_PIN, HIGH),
    1000, 2000,
    throttle.getMinSpeed(), throttle.getMaxSpeed()
  ));

  steering.setTargetPos(map(
    pulseIn(STEERING_RADIO_IN_PIN, HIGH),
    1000, 2000,
    -1 * steering.getMaxTurnDelta(), steering.getMaxTurnDelta()
  ));
}
