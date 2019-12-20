#include "obasi_const.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

Int64 generateID();
int generateClientSecret();
int generateRandNumber(long int);
static int isRandSeeded = FALSE;

void atexit_add(void* data, void*** varClean, int* varCleanCnt)
{
    void** tmp = *varClean;
    int tmpCnt = *varCleanCnt;
    tmp = realloc((*varClean), sizeof(void*) * (tmpCnt+1));
    tmp[tmpCnt++] = data;
    *varCleanCnt = tmpCnt;
    *varClean = tmp;
}

static void seedGenerator() {
    Time tv;
    gettimeofday(&tv, NULL);
    UInt64 msectime = (UInt64)(tv.tv_sec) * 100 + (UInt64)(tv.tv_usec);
    srandom(msectime);
    isRandSeeded = TRUE;
}

Int64 generateID() {
    if(isRandSeeded == FALSE) seedGenerator();
    Int32 id_1 = random();
    Int32 id_2 = random();
    Int64 id = (id_1 << 32) | id_2;
    return id;
}

Int32 generateID32() {
    if(isRandSeeded == FALSE) seedGenerator();
    return random();
}

int generateClientSecret() {
    return generateRandNumber(3000) + 1;
}

int generateRandNumber(long int limit) {
    if(isRandSeeded == FALSE) seedGenerator();
    return (random() % limit);
}

void generateRandNumberPlace(long int limit, int* place) {
    if(isRandSeeded == FALSE) seedGenerator();
    *place = random() % limit;
    return;
}

//Host-order to Network-order in 64bit
Int64 htonvl(Int64 hostnumvl) {
    Int32 msb = htonl( ((int) (hostnumvl >> 32)) );
    Int32 lsb = htonl( ((int) hostnumvl) );
    Int64 ret = ((lsb) << 32) | (msb);
    return ret;
}

//Network-order to Host-order in 64bit
Int64 ntohvl(Int64 netnumvl) {
    Int32 msb = ntohl( ((int) (netnumvl >> 32)) );
    Int32 lsb = ntohl( ((int) netnumvl) );
    Int64 ret = ((lsb) << 32) | (msb);
    return ret;
}

//Restituisce la differenza di tempo in millisecondi
int difftimeObasi(TimeSpec* finish, TimeSpec* start) {
    return (
        (1.0e3*((double)finish->tv_sec) + (1.0e-6*finish->tv_nsec)) -
        (1.0e3*((double)start->tv_sec) + (1.0e-6*start->tv_nsec))
    );
}
