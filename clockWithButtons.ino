#include <Arduino.h>
#include "SevenSeg.h"

//timekeeping for clock
#include "ardTimer.h"

//timer for blinking
#include "TimerThree.h"

//display
SevenSeg myDisplay (2,3,4,6,7,8,9);

const int numOfDigits=4;
int digitPins [numOfDigits]= {48,49,50,51};


//blinker
int digitPinsPair [numOfDigits]= {48,49,50,51};

int blinkEnable=0;
int firstBlinkDigit;
int lastBlinkDigit;

void blinkDigits();
void resetBlinker();


//what is this? i don't really know.
int hourSet;
int number=0;
int stopperTimeSec;
int stopperTimeMin;
int stopperTimeMillis;


//class for handling buttons
class button
{
    int pin;
    int value;
    int prevValue;

    int wentUp;
    int wentDown;
public:
    button(int pinPair)
    {
        pin=pinPair;
        wentDown=0;
        wentUp=0;

        pinMode(pin, INPUT);
    }

    int readState()
    {
        wentUp=0;
        wentDown=0;
        prevValue=value;
        value=digitalRead(pin);


        if (prevValue==LOW && value==HIGH) wentUp=1;
        if (prevValue==HIGH && value==LOW) wentDown=1;
        return value;
    }

    int getUpEdge()
    {
        return wentUp;
    }
    int getDownEdge()
    {
        return wentDown;
    }
    int getState()
    {
        return value;
    }
    void reset()
    {
        wentUp=0;
        wentDown=0;
    }

};

//more clock stuff
typedef enum mainStates
{
    SHOW,
    SET,
    STOPWATCH

} mainStates;

typedef enum setStates
{
    SETHOUR,
    SETMINUTE,
    SETSEC
} setStates;

typedef enum stopperStates
{
    RESET,
    RUN,
    PAUSE
} stopperStates;

int holdhour;
int holdminute;
int holdsec;

int increaseMinute(int number)
{
    if (number==59) number=0;
    else number++;
    return number;
}
int decreaseMinute(int number)
{
    if (number==0) number=59;
    else number--;
    return number;
}
int increaseHour(int number)
{
    if (number==23) number=0;
    else number++;
    return number;
}
int decreaseHour(int number)
{
    if (number==0) number=23;
    else number--;
    return number;
}




mainStates clockState;
setStates setState;
stopperStates stopperState;

//button stuff
button modeSelect(30);
button up(31);
button down(32);


void setup()
{

    //display stuff
    myDisplay.setDPPin(5);
    myDisplay.setDigitPins(numOfDigits,digitPins);
    myDisplay.setCommonCathode();
    myDisplay.setRefreshRate(20);
    myDisplay.setDutyCycle(10);

    //disp. timer stuff
    myDisplay.setTimer(1);
    myDisplay.startTimer();


    //button timer stuff
    cli();          // disable global interrupts
    TCCR2A = 0;     // set entire TCCR1A register to 0
    TCCR2B = 0;     // same for TCCR1B

    //set compare match register to desired timer count:
    OCR2A = 150;
    //turn on CTC mode:
    TCCR2A |= (1 << WGM12);
    //Set CS10 and CS12 bits for 1024 prescaler:
    TCCR2B |= (1 << CS10);
    TCCR2B |= (1 << CS12);
    //enable timer compare interrupt:
    TIMSK2 |= (1 << OCIE2A);
    //enable global interrupts:
    sei();

    //clock stuff
    setTime(12,1,0,8,3,2015);
    clockState=SHOW;
    setState=SETHOUR;
    stopperState=RESET;

    //timer for blinker
    Timer3.initialize(100000);
    Timer3.attachInterrupt(blinkDigits);
}

void loop()
{
    int i;
    switch (clockState)
    {
    case SHOW:
        holdhour=hour();
        holdminute=minute();
        holdsec=second();
        if (modeSelect.getUpEdge())
        {
            clockState=SET;
            modeSelect.reset();
        }
        myDisplay.writeClock(hour(),minute());
        break;
    case SET:
        switch (setState)
        {
        case SETHOUR:
            blinkEnable=1;
            firstBlinkDigit=0;
            lastBlinkDigit=1;
            myDisplay.writeClock(holdhour,holdminute);
            if (modeSelect.getUpEdge())
            {
                setState=SETMINUTE;
                clockState=SET;
                modeSelect.reset();
                resetBlinker();
            }
            if(up.getUpEdge())
            {
                holdhour=increaseHour(holdhour);
                up.reset();
            }
            else if(down.getUpEdge())
            {
                holdhour=decreaseHour(holdhour);
                down.reset();
            }
            setTime(holdhour,minute(),second(),day(),month(),year());
            break;
        case SETMINUTE:
            blinkEnable=1;
            firstBlinkDigit=2;
            lastBlinkDigit=3;
            myDisplay.writeClock(holdhour,holdminute);
            if (modeSelect.getUpEdge())
            {
                clockState=STOPWATCH;
                setState=SETHOUR;
                modeSelect.reset();
                resetBlinker();

            }
            if(up.getUpEdge())
            {
                holdminute=increaseMinute(holdminute);
                up.reset();
            }
            else if(down.getUpEdge())
            {
                holdminute=decreaseMinute(holdminute);
                down.reset();
            }
            setTime(hour(),holdminute,second(),day(),month(),year());
            break;
        }
        break;

    case STOPWATCH:
        switch(stopperState)
        {
        case RESET:
            blinkEnable=1;
            firstBlinkDigit=0;
            lastBlinkDigit=3;
            myDisplay.writeClock(0,0);
            if (modeSelect.getUpEdge())
            {
                clockState=SHOW;
                stopperState=RESET;
                modeSelect.reset();
                resetBlinker();
            }
            if (up.getUpEdge())
            {
                stopperState=RUN;
                resetBlinker();
                up.reset();

                //needs fix, this is not the way to do it
                stopperTimeSec=second();
                stopperTimeMin=minute();
            }
            else if(down.getUpEdge())
            {

                down.reset();
            }
            break;
        case RUN:

            myDisplay.writeClock(minute()-stopperTimeMin, second()-stopperTimeSec);
            if (modeSelect.getUpEdge())
            {
                clockState=SHOW;
                stopperState=RESET;
                modeSelect.reset();
                resetBlinker();
            }
            if(up.getUpEdge())
            {
                stopperState=RESET;
                up.reset();
            }
            else if(down.getUpEdge())
            {
                stopperState=PAUSE;
                down.reset();
                holdminute=minute()-stopperTimeMin;
                holdsec=second()-stopperTimeSec;
            }
            break;
        case PAUSE:
            myDisplay.writeClock(holdminute, holdsec);
            if (modeSelect.getUpEdge())
            {
                clockState=SHOW;
                stopperState=RESET;
                modeSelect.reset();
                resetBlinker();
            }
            if(up.getUpEdge())
            {
                stopperState=RESET;
                up.reset();
            }
            else if(down.getUpEdge())
            {
                stopperState=RUN;
                down.reset();
            }
            break;
        }
        break;

    }
}

ISR( TIMER1_COMPA_vect )
{
    myDisplay.interruptAction();
}

ISR(TIMER2_COMPA_vect)
{
    modeSelect.readState();
    up.readState();
    down.readState();
}

void blinkDigits()
{
    if (!blinkEnable) return;
    int i;
    if (digitPins[firstBlinkDigit]==digitPinsPair[firstBlinkDigit])
    {
        for (i=firstBlinkDigit; i<=lastBlinkDigit; i++)
        {
            digitPins[i]=99;
            digitalWrite(digitPinsPair[i], HIGH);
        }
    }
    else
    {
        for (i=firstBlinkDigit; i<=lastBlinkDigit; i++)
        {
            digitPins[i]=digitPinsPair[i];
        }

    }

}

void resetBlinker()
{
    int i;
    blinkEnable=0;
    for (i=firstBlinkDigit; i<=lastBlinkDigit; i++)
    {
        digitPins[i]=digitPinsPair[i];
    }
}


//    case SETSEC:
//        myDisplay.writeClock(holdhour,holdminute);
//        if (modeSelect.getUpEdge())
//        {
//            clockState=SHOW;
//        }
//        if(up.getUpEdge())
//        {
//            increase(holdsecond);
//            up.reset();
//        }
//        else if(down.getUpEdge())
//        {
//            decrease(holdsecond);
//            down.reset();
//        }
//
//        setTime(hour(),minute(), holdsec,day(),month(),year());
//        break;
