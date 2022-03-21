class Throttle {
private:
  int speed = 0;

public:
  int getSpeed() { return speed; }

  void setSpeed(int newSpeed) {
    speed = newSpeed;
  }

  void update() {
    analogWrite(THROTTLE_PIN, speed);
  }

  void stop() {
    setSpeed(0);
  }
};
