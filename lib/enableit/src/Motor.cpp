// Motor.cpp — implementation moved to PQ12Actuator.cpp
// This file is intentionally empty.

// suppress empty translation unit warning
namespace { constexpr int _motor_cpp_placeholder = 0; }

#if 0  // archived
void _motor_cpp_archived()
{
}

void Motor::init(int in1Pin, int in2Pin, int wiperPin, int isensePin, bool enableSpeed)
{
    this->in1Pin = in1Pin;
    this->in2Pin = in2Pin;
    this->wiperPin = wiperPin;
    this->isensePin = isensePin;
    log_d("Init motor");
    speedControl = enableSpeed;

    // Configura i pin come output/input
    pinMode(in1Pin, OUTPUT);
    pinMode(in2Pin, OUTPUT);


#if defined(DRIVER_DRV8411) || defined(DRIVER_DRV8833)
    if (speedControl)
    {
        analogWriteResolution(MOTOR_SPEED_BIT);
        analogWriteFrequency(MOTOR_PWM_FREQ);
    }
#endif

#ifdef MOTOR_FAULT
    pinMode(MOTOR_FAULT, INPUT_PULLUP);
#endif

#if defined(DRIVER_DRV8835) || defined(DRIVER_DRV8833)
    // enable not used in DRV8411
    // set pin output
    pinMode(MOTOR_ENABLE, OUTPUT);
    digitalWrite(MOTOR_ENABLE, TRUE);
#endif

    if (isensePin != -1) {
        pinMode(isensePin, INPUT);
        log_d("Motor current sense pin configured as input");
    }
    if (wiperPin != -1) {
        pinMode(wiperPin, INPUT);
        log_d("Motor position wiper pin configured as input");
        position = analogRead(wiperPin);
        log_d("Motor position wiper initial value: %d", position);
    } else {
        position = 0; // Default position if wiperPin is not set
        log_d("Motor position wiper pin not configured, using default position: %d", position);
    }

    log_d("Motor initialized");

    stop(String("Init"));
    current = current_max = 0;
    poll();
}

void Motor::speed(int speed)
{

    int pwm_speed = abs(speed);
    if (pwm_speed > MOTOR_SPEED_MAX)
        pwm_speed = MOTOR_SPEED_MAX;

    log_d("Motor speed: %d", pwm_speed);

    if (speed > 0)
        direction = FORWARD;
    else if (speed < 0)
        direction = REVERSE;
    else
        stop("user stop");

    poll();

    if (direction == FORWARD)
        forward(pwm_speed);
    else if (direction == REVERSE)
        reverse(pwm_speed);
}

void Motor::forward(int pwm_speed)
{
#ifdef DRIVER_DRV8835
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
    log_d("Motor forward");
#else // DRV8411, DRV8833
    if (speedControl)
    {
#ifdef MOTOR_FASTDECAY_MODE
        // forward fast decay IN1(pwm), IN2(0)
        analogWrite(in1Pin, pwm_speed);
        analogWrite(in2Pin, 0);
        log_d("Motor forward, fast, speed: %d", pwm_speed);
#else
        // forward slow decay IN1(1), IN2(pwm)
        analogWrite(in1Pin, MOTOR_SPEED_MAX);
        analogWrite(in2Pin, pwm_speed);
        log_d("Motor forward, slow, speed: %d", pwm_speed);
#endif
    }
    else
    {
        digitalWrite(in1Pin, 1);
        digitalWrite(in2Pin, 0);
        log_d("Motor forward");
    }
#endif
}

void Motor::reverse(int pwm_speed)
{
#ifdef DRIVER_DRV8835
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
#else // DRV8411, DRV8833
    if (speedControl)
    {
#ifdef MOTOR_FASTDECAY_MODE
        // reverse fast decay IN1(0), IN2(pwm)
        analogWrite(in1Pin, 0);
        analogWrite(in2Pin, pwm_speed);
        log_d("Motor reverse, fast, speed: %d", pwm_speed);
#else
        // reverse slow decay IN1(pwm), IN2(1)
        analogWrite(in1Pin, pwm_speed);
        analogWrite(in2Pin, MOTOR_SPEED_MAX);
        log_d("Motor reverse, slow, speed: %d", pwm_speed);
#endif
    }
    else
    {
        digitalWrite(in1Pin, 0);
        digitalWrite(in2Pin, 1);
        log_d("Motor reverse");
    }
#endif

}

void Motor::stop(String message)
{

#ifdef DRIVER_DRV8835
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, HIGH);
#else // DRV8411, DRV8833
    if (speedControl)
    {
        analogWrite(in1Pin, MOTOR_SPEED_MAX);
        analogWrite(in2Pin, MOTOR_SPEED_MAX);
    }
    else
    {
        digitalWrite(in1Pin, 1);
        digitalWrite(in2Pin, 1);
    }
#endif    
    log_d("Motor stop");

    direction = STOP;
    log_d("Motor stop: %s", message.c_str());
}

void Motor::sleep()
{
#if defined(DRIVER_DRV8835) || defined(DRIVER_DRV8833)
    digitalWrite(MOTOR_ENABLE, LOW);
    log_d("Motor sleep");
#else
    digitalWrite(in1Pin, 0);
    digitalWrite(in2Pin, 0);
    log_d("Motor sleep");
#endif
}

void Motor::wakeup()
{
#if defined(DRIVER_DRV8835) || defined(DRIVER_DRV8833)
    digitalWrite(MOTOR_ENABLE, HIGH);
    log_d("Motor wake");
#endif
}

int Motor::getPosition()
{   
    if (wiperPin != -1) {
        position = analogRead(wiperPin);
        log_d("Motor position: %d", position);
        return position;
    } else {
        log_d("Motor position not available");
        return 0;
    }
}

} // end archived
#endif  // archived