#include <Motor.h>
#include <Console.h>

Motor PQ12Motor;

Motor::Motor()
{
}

void Motor::init(bool enableSpeed)
{
    DBG("Init motor");

    speedControl = enableSpeed;

    // common setup, set pin output
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);

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

#ifdef MOTOR_ISENSE
    pinMode(MOTOR_ISENSE, INPUT);
    DBG("Motor current sense pin configured as input");
#endif
    
    DBG("Motor initialized");

    stop(String("Init"));

    current = current_max = 0;

    poll();
}

void Motor::speed(int speed)
{

    int pwm_speed = abs(speed);
    if (pwm_speed > MOTOR_SPEED_MAX)
        pwm_speed = MOTOR_SPEED_MAX;

    DBG("Motor speed: %d", pwm_speed);

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
    digitalWrite(MOTOR_IN1, 1);
    digitalWrite(MOTOR_IN2, 0);
#else // DRV8411, DRV8833
    if (speedControl)
    {
#ifdef MOTOR_FASTDECAY_MODE
        // forward fast decay IN1(pwm), IN2(0)
        analogWrite(MOTOR_IN1, pwm_speed);
        analogWrite(MOTOR_IN2, 0);
        DBG("Motor forward, fast, speed: %d", pwm_speed);
#else
        // forward slow decay IN1(1), IN2(pwm)
        analogWrite(MOTOR_IN1, MOTOR_SPEED_MAX);
        analogWrite(MOTOR_IN2, pwm_speed);
        DBG("Motor forward, slow, speed: %d", pwm_speed);
#endif
    }
    else
    {
        digitalWrite(MOTOR_IN1, 1);
        digitalWrite(MOTOR_IN2, 0);
        DBG("Motor forward");
    }
#endif
}

void Motor::reverse(int pwm_speed)
{
#ifdef DRIVER_DRV8835
    digitalWrite(MOTOR_IN1, 1);
    digitalWrite(MOTOR_IN2, 1);
    DBG("Motor reverse");
#else // DRV8411, DRV8833
    if (speedControl)
    {
#ifdef MOTOR_FASTDECAY_MODE
        // reverse fast decay IN1(0), IN2(pwm)
        analogWrite(MOTOR_IN1, 0);
        analogWrite(MOTOR_IN2, pwm_speed);
        DBG("Motor reverse, fast, speed: %d", pwm_speed);
#else
        // reverse slow decay IN1(pwm), IN2(1)
        analogWrite(MOTOR_IN1, pwm_speed);
        analogWrite(MOTOR_IN2, MOTOR_SPEED_MAX);
        DBG("Motor reverse, slow, speed: %d", pwm_speed);
#endif
    }
    else
    {
        digitalWrite(MOTOR_IN1, 0);
        digitalWrite(MOTOR_IN2, 1);
        DBG("Motor reverse");
    }
#endif
}

void Motor::stop(String message)
{

#ifdef DRIVER_DRV8835
    digitalWrite(MOTOR_IN1, 0);
    digitalWrite(MOTOR_IN2, 0);
#else // DRV8411, DRV8833
    if (speedControl)
    {
        analogWrite(MOTOR_IN1, MOTOR_SPEED_MAX);
        analogWrite(MOTOR_IN2, MOTOR_SPEED_MAX);
    }
    else
    {
        digitalWrite(MOTOR_IN1, 1);
        digitalWrite(MOTOR_IN2, 1);
    }
#endif    
    DBG("stop");

    direction = STOP;
    DBG("Motor stop: %s", message.c_str());
}

void Motor::sleep()
{
#if defined(DRIVER_DRV8835) || defined(DRIVER_DRV8833)
    digitalWrite(MOTOR_ENABLE, LOW);
    DBG("Motor sleep");
#else
    digitalWrite(MOTOR_IN1, 0);
    digitalWrite(MOTOR_IN2, 0);
    DBG("Motor sleep");
#endif
}

void Motor::wakeup()
{
#if defined(DRIVER_DRV8835) || defined(DRIVER_DRV8833)
    digitalWrite(MOTOR_ENABLE, HIGH);
    DBG("Motor wake");
#endif
}

void Motor::poll()
{
#ifdef MOTOR_POSITION_PROTECTION
    position = analogRead(MOTOR_WIPER);
    if ((position > MOTOR_HIGH_THRESHOLD) && (direction == FORWARD))
    {
        stop("Poll, reverse motor protection enabled");
        return;
    }
    else if ((position < MOTOR_LOW_THRESHOLD) && (direction == REVERSE))
    {
        stop("Poll, forward motor protection enabled");
        return;
    }
#endif
#ifdef MOTOR_CURRENT_PROTECTION
    current = analogRead(MOTOR_ISENSE);

    
    if (current > current_max) {
        current_max = current;
        DBG("Motor current(%d), max(%d)", current, current_max);
    }

#ifdef CURRENT_LIMIT
    if (current > MOTOR_CURRENT_THRESHOLD)
    {
        stop("Poll, current limit reached");
        return;
    }
#endif
#endif
}