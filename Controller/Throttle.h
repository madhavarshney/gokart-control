class Throttle {
private:
  int speed = 0;

public:
  int getSpeed() { return speed; }

  void setSpeed(int newSpeed) {
    speed = newSpeed;
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
