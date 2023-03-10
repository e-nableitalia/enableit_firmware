#ifndef E_CONF
#define E_CONF

#include "Arduino.h"

#define PERIOD 1000
//Channel
#define WINDOWLENGTH 200
#define STEPSIZE 75
#define SAMPLESIZE 2000
#define NUM_CHANNELS 8
 
//ADS
#define ADS_RESET GPIO_NUM_0
#define ADS_DRDY GPIO_NUM_38
#define ADS_CS GPIO_NUM_39
/*
pwdn 3
rset 2
start 4
drdy 5
cs 10
*/
//LDA
#define NUMCLASSES 5
#define NUMFEATS (NUM_CHANNELS * 4)
#define PREVIOUSCLASSES 8
#define MLDA_ADDRESS 3
#define CLDA_ADDRESS 1024
#define STATUS_ADDRESS 2
#define THRESHOLD_MULT 1.15

//Proportional Control
#define PMINTHRESH 10
#define PMAXTHRESH 70

#endif
