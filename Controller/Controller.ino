#include "Steering.h"
#include "Throttle.h"

constexpr size_t STATUS_LED_PIN          = 13;
constexpr size_t THROTTLE_PIN            = 11;
constexpr size_t STEERING_RIGHT_PIN      = 9;
constexpr size_t STEERING_LEFT_PIN       = 10;
constexpr size_t STEERING_TEST_SERVO_PIN = 9;
constexpr size_t STEERING_POS_IN_PIN     = A0;

constexpr size_t THROTTLE_RADIO_IN_PIN = 7;
constexpr size_t STEERING_RADIO_IN_PIN = 8;

// Mode to use testing peripherals: 
// throttle - LED on PWM pin, steering - micro servo
bool useTestPeripherals = false;

bool radioMode = false;
bool requireCommandStream = true;

unsigned long maxWaitTime = 3000; // 3 seconds
unsigned long lastCommandTime = 0;

Throttle throttle;
Steering steering;

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(THROTTLE_RADIO_IN_PIN, INPUT);
  pinMode(STEERING_RADIO_IN_PIN, INPUT);

  throttle.setup(THROTTLE_PIN, useTestPeripherals);

  if (useTestPeripherals)
    steering.setupTestServo(STEERING_TEST_SERVO_PIN);
  else
    steering.setup(
      STEERING_RIGHT_PIN,
      STEERING_LEFT_PIN,
      STEERING_POS_IN_PIN
    );

  // Set min/max for throttle speed
  throttle.setMinMaxSpeed(80, 110);
  // Calibrate with center analog reading, max left/right turn amount
  steering.calibrate(520, 185);

  Serial.begin(115200);
  while (!Serial)
    ; // Wait for serial port to connect. Needed for Native USB only
  Serial.println("Listening for commands");
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

  digitalWrite(STATUS_LED_PIN, isStopped ? LOW : HIGH);

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
