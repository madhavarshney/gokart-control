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

  int steeringSpeed = 100;
  int stopDelta = 20;

  int forceStop = false;
  int currentPos = 0;
  int targetPos = 0;

  Servo *testServo = nullptr;

  Direction getDirection() {
    if (forceStop || testServo)
      return STOP;

    currentPos = analogRead(posPin);

    // safeguards
    if (currentPos < minPos)
      return RIGHT;
    else if (currentPos > maxPos)
      return LEFT;

    // based on target position
    if (abs(currentPos - targetPos) < stopDelta)
      return STOP;
    else if (currentPos > targetPos)
      return LEFT;
    else if (currentPos < targetPos)
      return RIGHT;
  }

public:
  void setup(int _rightPin, int _leftPin, int _posPin) {
    _rightPin = rightPin;
    _leftPin = leftPin;
    _posPin = posPin;

    pinMode(rightPin, OUTPUT);
    pinMode(leftPin, OUTPUT);
  }

  void setupTestServo(int testServoPin) {
    testServo = new Servo();
    testServo->attach(testServoPin);
  }

  void calibrate(int _centerPos, int _maxTurnDelta) {
    centerPos = _centerPos;
    maxTurnDelta = _maxTurnDelta;
    minPos = centerPos - maxTurnDelta;
    maxPos = centerPos + maxTurnDelta;
    targetPos = centerPos;
  }

  int getCurrentPos() { return currentPos; }
  int getTargetPos() { return targetPos; }
  int getOffsetPos() { return targetPos - centerPos; } // TODO: rename method

  int getCenterPos() { return centerPos; }
  int getMaxTurnDelta() { return maxTurnDelta; }
  int getMinPos() { return minPos; }
  int getMaxPos() { return maxPos; }

  bool isStopped() { return getDirection() == STOP; }

  void setTargetPos(int newTargetPos) {
    forceStop = false;
    targetPos = min(max(centerPos + newTargetPos, minPos), maxPos);
  }

  void update() {
    Direction direction = getDirection();

    if (testServo) {
      testServo->write(map(
        getOffsetPos(),
        -1 * maxTurnDelta, maxTurnDelta,
        0, 180
      ));
      return;
    }

    if (direction == LEFT) {
      analogWrite(leftPin, steeringSpeed);
      analogWrite(rightPin, 0);
    } else if (direction == RIGHT) {
      analogWrite(leftPin, 0);
      analogWrite(rightPin, steeringSpeed);
    } else {
      analogWrite(leftPin, 0);
      analogWrite(rightPin, 0);
    }
  }

  void stop() {
    forceStop = true;
  }
};
