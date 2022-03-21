class Throttle {
private:
  int speed = 0;
  int minSpeed;
  int maxSpeed;

public:
  int getSpeed() { return speed; }
  int getMinSpeed() { return minSpeed; }
  int getMaxSpeed() { return maxSpeed; }

  void setSpeed(int newSpeed) {
    speed = newSpeed;

    if (speed > maxSpeed)
      speed = maxSpeed;
    else if (speed < minSpeed)
      speed = 0;
  }

  void setMinMaxSpeed(int _minSpeed, int _maxSpeed) {
    minSpeed = _minSpeed;
    maxSpeed = _maxSpeed;
  }

  bool isStopped() {
    return speed == 0;
  }

  void update() {
    analogWrite(THROTTLE_PIN, speed);
  }

  void stop() {
    setSpeed(0);
  }
};
