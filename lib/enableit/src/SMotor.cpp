#include "Console.h"
#include "SMotor.h"

HardwareSerial *SMotor::scSerial = nullptr;

SMotor::SMotor()
    : id(0), lastPosition(-1), SMS_STS()
{
}

void SMotor::init(uint8_t servoId)
{
    id = servoId;
    pSerial = scSerial;
}

void SMotor::static_init(int sRX, int sTX)
{
    scSerial = &Serial2;
    log_d("Initializing Serial2 for SMotor");
    if (scSerial == nullptr) {
        log_d("Seriale non inizializzata!");
        return;
    }
    //scSerial->begin(1000000, SERIAL_8N1, sRX, sTX); // Set baudrate and pins
}

void SMotor::speed(int speed)
{
    // Imposta la velocità del servo
    // La posizione rimane invariata
    WritePosEx(id, getPosition(),speed, 0);
}

void SMotor::move(int position)
{
    // Muove il servo alla posizione specificata
    WritePosEx(id, position, 0, 0);
    lastPosition = position;
}

void SMotor::forward(int speed)
{
    // Muove il servo in avanti (verso posizione massima)
    WritePosEx(id, 1023, speed, 0); // 1023 = posizione massima tipica
    lastPosition = 1023;
}

void SMotor::reverse(int speed)
{
    // Muove il servo indietro (verso posizione minima)
    WritePosEx(id, 0, speed, 0);
    lastPosition = 0;
}

void SMotor::stop(String message)
{
    // Disabilita la coppia (torque)
    EnableTorque(id, 0);
}

void SMotor::sleep()
{
    // Disabilita la coppia (torque)
    EnableTorque(id, 0);
}

void SMotor::wakeup()
{
    // Abilita la coppia (torque)
    EnableTorque(id, 1);
}

void SMotor::poll()
{
    // Aggiorna la posizione
    lastPosition = getPosition();
}

int SMotor::getPosition()
{
    // Legge la posizione attuale del servo
    return ReadPos(id);
}

int SMotor::getCurrent()
{
    // Legge la corrente assorbita dal servo
    return ReadCurrent(id);
}

int SMotor::getError()
{
    // Legge l'ultimo errore del servo
    return Error;
}

