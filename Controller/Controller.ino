// For Arduino Nano 33 IoT (experimental)
// #define ENABLE_WIFI_SERVER true;

#include "Steering.h"
#include "Throttle.h"
#include "ArduinoJson.h"

#ifdef ENABLE_WIFI_SERVER
#include "WiFiInput.h"
#endif

constexpr size_t STATUS_LED_PIN          = 13;
constexpr size_t THROTTLE_PIN            = 11;
constexpr size_t STEERING_RIGHT_PIN      = 10;
constexpr size_t STEERING_LEFT_PIN       = 9;
constexpr size_t STEERING_TEST_SERVO_PIN = 10;
constexpr size_t STEERING_POS_IN_PIN     = A0;

constexpr size_t THROTTLE_RADIO_IN_PIN = 3;
constexpr size_t STEERING_RADIO_IN_PIN = 2;

constexpr size_t ENABLE_PIN = 8;
constexpr size_t RADIO_TOGGLE_PIN = 7;
constexpr size_t MANUAL_TOGGLE_PIN = 6;

// Mode to use testing peripherals:
// throttle - LED on PWM pin, steering - micro servo
bool useTestPeripherals = false;

bool enableThrottleRadioIn = false;
bool enableSteeringRadioIn = true;

// Pulse Widths
volatile int radioThrottlePW = 1500;
volatile int radioSteeringPW = 1500;

bool requireCommandStream = true;

bool isStopped = true;
bool radioMode = false;
bool motorEnabled = false;
bool manualMode = true;

unsigned long maxWaitTime = 3000; // 3 seconds
unsigned long lastCommandTime = 0;

unsigned long currentMillis = 0;
unsigned long stateUpdateInterval = 100; // 100 ms
unsigned long lastStateUpdateTime = 0;

Throttle throttle;
Steering steering;

#ifdef ENABLE_WIFI_SERVER
WiFiInput wifi(throttle, steering);
#endif

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);

  if (enableThrottleRadioIn) {
    pinMode(THROTTLE_RADIO_IN_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(THROTTLE_RADIO_IN_PIN), onThrottleRadioInterrupt, CHANGE);
  }

  if (enableSteeringRadioIn) {
    pinMode(STEERING_RADIO_IN_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(STEERING_RADIO_IN_PIN), onSteeringRadioInterrupt, CHANGE);
  }

  pinMode(ENABLE_PIN, INPUT_PULLUP);
  pinMode(RADIO_TOGGLE_PIN, INPUT_PULLUP);
  pinMode(MANUAL_TOGGLE_PIN, INPUT_PULLUP);

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
  throttle.setMinMaxSpeed(70, 110);
  // Calibrate with center analog reading, max left/right turn amount,
  // and stop window
  steering.calibrate(500, 200, 20);
  // Set min/max steering motor speed
  steering.setMinMaxSpeed(105, 240);

  Serial.begin(115200);
  while (!Serial)
    ; // Wait for serial port to connect. Needed for Native USB only
  Serial.println("Listening for commands");

  #ifdef ENABLE_WIFI_SERVER
  wifi.setup();
  #endif
}

void loop() {
  currentMillis = millis();
  isStopped = steering.isStopped() && throttle.isStopped();

  motorEnabled = digitalRead(ENABLE_PIN);
  radioMode = digitalRead(RADIO_TOGGLE_PIN);
  manualMode = digitalRead(MANUAL_TOGGLE_PIN);

  throttle.setMotorEnabled(motorEnabled && !manualMode);
  steering.setMotorEnabled(motorEnabled && !manualMode);

  #ifdef ENABLE_WIFI_SERVER
  lastCommandTime = wifi.handleInput();
  #endif

  if (!manualMode) {
    if (radioMode) {
      handleRadioInput();
    } else if (Serial.available() > 0) {
      lastCommandTime = currentMillis;
      handleSerialInput();
    } else if (
      requireCommandStream
      && currentMillis - lastCommandTime > maxWaitTime
      && !isStopped
    ) {
      // Stop if no command is received
      stop();
    }
  } else if (!isStopped) {
    stop();
  }

  digitalWrite(STATUS_LED_PIN, isStopped ? LOW : HIGH);

  steering.update();
  throttle.update();

  // TODO: check if re-setting currentMillis has unintended consequences
  // with the state updates
  currentMillis = millis();

  if (currentMillis - lastStateUpdateTime > stateUpdateInterval) {
    lastStateUpdateTime = currentMillis;
    sendStateUpdate();
  }

  // delay(50);
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
}

/**
 * Scale radio control input and set values
 */
void handleRadioInput() {
  if (enableThrottleRadioIn) {
    throttle.setSpeed(map(
      radioThrottlePW, 1000, 2000,
      throttle.getMinSpeed(), throttle.getMaxSpeed()
    ));
  }

  if (enableSteeringRadioIn) {
    steering.setTargetPos(map(
      radioSteeringPW, 1000, 2000,
      -1 * steering.getMaxTurnDelta(), steering.getMaxTurnDelta()
    ));
  }
}

void onThrottleRadioInterrupt() {
  static volatile unsigned long timerStart = 0;
  static volatile int lastInterruptTime = 0;

  handleRadioInterrupt(
    THROTTLE_RADIO_IN_PIN, radioThrottlePW,
    timerStart, lastInterruptTime
  );
}

void onSteeringRadioInterrupt() {
  static volatile unsigned long timerStart = 0;
  static volatile int lastInterruptTime = 0;

  handleRadioInterrupt(
    STEERING_RADIO_IN_PIN, radioSteeringPW,
    timerStart, lastInterruptTime
  );
}

void handleRadioInterrupt(
  int pin,
  volatile int &pulseWidth,
  volatile unsigned long &timerStart,
  volatile int &lastInterruptTime
) {
  lastInterruptTime = micros();

  if (digitalRead(pin) == HIGH) {
    timerStart = micros();
  } else {
    if (timerStart != 0) {
      pulseWidth = ((volatile int)micros() - timerStart);
      timerStart = 0;
    }
  }
}

void sendStateUpdate() {
  StaticJsonDocument<384> doc;

  JsonObject generalDoc = doc.createNestedObject("general");
  generalDoc["millis"] = currentMillis;
  generalDoc["motorEnabled"] = motorEnabled;
  generalDoc["mode"] = manualMode ? "manual" : radioMode ? "radio" : "phone";
  generalDoc["isStopped"] = isStopped;

  JsonObject throttleDoc = doc.createNestedObject("throttle");
  throttleDoc["speed"] = throttle.getSpeed();
  throttleDoc["minSpeed"] = throttle.getMinSpeed();
  throttleDoc["maxSpeed"] = throttle.getMaxSpeed();

  JsonObject steeringDoc = doc.createNestedObject("steering");
  steeringDoc["targetPos"] = steering.getTargetPos();
  steeringDoc["currentPos"] = steering.getCurrentPos();
  steeringDoc["centerPos"] = steering.getCenterPos();
  steeringDoc["maxTurnDelta"] = steering.getMaxTurnDelta();
  steeringDoc["speed"] = steering.getSpeed();
  steeringDoc["minSpeed"] = steering.getMinSpeed();
  steeringDoc["maxSpeed"] = steering.getMaxSpeed();
  steeringDoc["stopDelta"] = steering.getStopDelta();

  JsonObject radioDoc = doc.createNestedObject("radio");

  if (enableThrottleRadioIn)
    radioDoc["throttle"] = radioThrottlePW;
  if (enableSteeringRadioIn)
    radioDoc["steering"] = radioSteeringPW;

  serializeJson(doc, Serial);
  Serial.print("\n");
}
