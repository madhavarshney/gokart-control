#include <Servo.h>

class Steering {
private:
  enum Direction {
    LEFT, RIGHT, STOP
  };

  int leftPin = -1;
  int rightPin = -1;
  int posPin = -1;

  int centerPos = 0;
  int maxTurnDelta = 0;
  int minPos = 0;
  int maxPos = 0;
  int maxSpeedThreshold = 0;

  int speed = 0;
  int minSpeed = 0;
  int maxSpeed = 0;
  int stopDelta = 20;

  bool motorEnabled = false;
  bool forceStop = false;

  int currentPos = 0;
  int targetPos = 0;
  int currentPosError = 0;

  Servo *testServo = nullptr;

  void updateCurrentPos() {
    currentPos = analogRead(posPin);
    currentPosError = abs(targetPos - currentPos);
  }

  void updateSpeed() {
    float speedFraction = min((float) currentPosError / maxSpeedThreshold, 1);
    speed = minSpeed + speedFraction * (maxSpeed - minSpeed);
  }

  Direction getDirection() {
    if (!motorEnabled || forceStop || testServo)
      return STOP;

    // safeguards
    if (currentPos < minPos)
      return RIGHT;
    else if (currentPos > maxPos)
      return LEFT;

    // based on target position
    if (currentPosError < stopDelta)
      return STOP;
    else if (currentPos > targetPos)
      return LEFT;
    else if (currentPos < targetPos)
      return RIGHT;
  }

public:
  void setup(int _rightPin, int _leftPin, int _posPin) {
    rightPin = _rightPin;
    leftPin = _leftPin;
    posPin = _posPin;

    pinMode(rightPin, OUTPUT);
    pinMode(leftPin, OUTPUT);
  }

  void setupTestServo(int testServoPin) {
    testServo = new Servo();
    testServo->attach(testServoPin);
  }

  void calibrate(int _centerPos, int _maxTurnDelta, int _stopDelta) {
    centerPos = _centerPos;
    maxTurnDelta = _maxTurnDelta;
    minPos = centerPos - maxTurnDelta;
    maxPos = centerPos + maxTurnDelta;
    targetPos = centerPos;
    stopDelta = _stopDelta;
    maxSpeedThreshold = 0.5 * (maxPos - minPos); // TODO: make configurable
  }

  void setMinMaxSpeed(int _minSpeed, int _maxSpeed) {
    minSpeed = _minSpeed;
    maxSpeed = _maxSpeed;
  }

  int getCurrentPos() { return currentPos; }
  int getTargetPos() { return targetPos; }
  int getOffsetPos() { return targetPos - centerPos; } // TODO: rename method

  int getCenterPos() { return centerPos; }
  int getMaxTurnDelta() { return maxTurnDelta; }
  int getMinPos() { return minPos; }
  int getMaxPos() { return maxPos; }

  int getSpeed() { return speed; }
  int getMinSpeed() { return minSpeed; }
  int getMaxSpeed() { return maxSpeed; }
  int getStopDelta() { return stopDelta; }

  bool isStopped() { return getDirection() == STOP; }
  bool isMotorEnabled() { return motorEnabled; }

  void setMotorEnabled(bool enabled) { motorEnabled = enabled; }

  void setTargetPos(int newTargetPos) {
    forceStop = false;
    targetPos = min(max(centerPos + newTargetPos, minPos), maxPos);
  }

  void update() {
    updateCurrentPos();
    updateSpeed();

    if (testServo) {
      testServo->write(map(
        getOffsetPos(),
        -1 * maxTurnDelta, maxTurnDelta,
        0, 180
      ));
      return;
    }

    Direction direction = getDirection();

    if (direction == LEFT) {
      analogWrite(leftPin, speed);
      analogWrite(rightPin, 0);
    } else if (direction == RIGHT) {
      analogWrite(leftPin, 0);
      analogWrite(rightPin, speed);
    } else {
      analogWrite(leftPin, 0);
      analogWrite(rightPin, 0);
    }
  }

  void stop() {
    forceStop = true;
  }
};
