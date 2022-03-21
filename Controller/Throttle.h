class Throttle {
private:
  int pin = -1;
  bool useTestLed = false;

  int speed = 0;
  int minSpeed = 0;
  int maxSpeed = 0;

public:
  void setup(int _pin, bool _useTestLed) {
    pin = _pin;
    useTestLed = _useTestLed;
    pinMode(pin, OUTPUT);
  }

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
    analogWrite(
      pin,
      useTestLed
        ? map(
            speed >= minSpeed ? speed - minSpeed + 1 : 0,
            0, maxSpeed - minSpeed + 5,
            0, 255
          )
        : speed
    );
  }

  void stop() {
    setSpeed(0);
  }
};
