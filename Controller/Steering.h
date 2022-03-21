class Steering {
private:
  enum Direction {
    LEFT, RIGHT, STOP
  };

  int centerPos;
  int maxTurnDelta;
  int minPos;
  int maxPos;

  int steeringSpeed = 100;
  int stopDelta = 20;

  int forceStop = false;
  int currentPos = centerPos;
  int targetPos = centerPos;

  Direction getDirection() {
    if (forceStop)
      return STOP;

    currentPos = analogRead(STEERING_POS_PIN);

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
  void calibrate(int _centerPos, int _maxTurnDelta) {
    centerPos = _centerPos;
    maxTurnDelta = _maxTurnDelta;
    minPos = centerPos - maxTurnDelta;
    maxPos = centerPos + maxTurnDelta;
  }

  int getCurrentPos() { return currentPos; }
  int getTargetPos() { return targetPos; }
  int getOffsetPos() { return targetPos - centerPos; } // TODO: rename method

  bool isStopped() { return getDirection() == STOP; }

  void setTargetPos(int newTargetPos) {
    forceStop = false;
    targetPos = min(max(centerPos + newTargetPos, minPos), maxPos);
  }

  void update() {
    Direction direction = getDirection();

    if (direction == LEFT) {
      analogWrite(STEERING_LEFT_PIN, steeringSpeed);
      analogWrite(STEERING_RIGT_PIN, 0);
    } else if (direction == RIGHT) {
      analogWrite(STEERING_LEFT_PIN, 0);
      analogWrite(STEERING_RIGT_PIN, steeringSpeed);
    } else {
      analogWrite(STEERING_LEFT_PIN, 0);
      analogWrite(STEERING_RIGT_PIN, 0);
    }
  }

  void stop() {
    forceStop = true;
  }
};
