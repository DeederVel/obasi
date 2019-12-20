/*
    Let's roll cause it's a Dami flying that one over there
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "obasi_const.h"
#include "commons.c"

void **varClean = NULL;
int varCleanCnt = 0;
void cleanup() {
    int i;
    for(i = 0; i<varCleanCnt; i++) {
        free(varClean[i]);
    }
    free(varClean);
}
void sigHandler() {
    exit(0);
}

int main(int argc, char* argv[]) {
    atexit(cleanup);
    SigAct sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = sigHandler;
    sigaction(SIGINT,&sigact,NULL);

    int k, p, w, i;
    if(argc == 1) {
        printf("Inserire il parametro k:\n> ");
        scanf("%d", &k);
        do {
            printf("Inserire il parametro p (compreso tra 1 e k):\n> ");
            scanf("%d", &p);
        } while (p < 1 || p >= k);
        int wLimit = p*3;
        do {
            printf("Inserire il parametro w (>3p):\n> ");
            scanf("%d", &w);
        } while (w <= wLimit);
    } else {
        if(argc == 4) {
            p = atoi(argv[1]);
            k = atoi(argv[2]);
            w = atoi(argv[3]);
            if(p < 1 || p >= k) {
                printf("Il parametro p deve essere compreso tra 1 e k-1\n");
                exit(EXIT_FAILURE);
            }
            int wLimit = p*3;
            if(w <= wLimit) {
                printf("Il parametro w deve essere superiore a 3p (tre volte p)\n");
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Sintassi avvio: ./client <p> <k> <w> o ./client \n");
            exit(EXIT_FAILURE);
        }
    }

    Int64 id = generateID();
    Int64 netID = htonvl(id);
    char netIDstring[INT64_DECREP];
    memset(netIDstring, 0, INT64_DECREP);
    sprintf(netIDstring, "%lld", netID);
    int secret = generateClientSecret();
    printf("CLIENT %llx SECRET %d\n", id, secret);
    fflush(stdout);
    int* servers = malloc(p*sizeof(int));
    atexit_add(servers, &varClean, &varCleanCnt);
    int* serversFD = malloc(p*sizeof(int));
    atexit_add(serversFD, &varClean, &varCleanCnt);

    /*
        Implementazione variante algoritmo X di Knuth
        range k
        length p
    */
    int ik, ip;
    ip = 0;
    for(ik = 1; ik <= k && ip < p; ++ik)  {
        int rk = k - ik;
        int rp = p - ip;
        if(generateRandNumber(rk) < rp)
            servers[ip++] = ik;
    }

    struct sockaddr_un sock_addr;
    char* baseservname = SOCK_PATH; //No free, constant
    int servnameLen = strlen(baseservname)+sizeof(int);
    char* servername = malloc(servnameLen*sizeof(char));
    atexit_add(servername, &varClean, &varCleanCnt);
    sock_addr.sun_family=AF_UNIX;

    socket(AF_UNIX,SOCK_STREAM,0);
    for(i=0; i<p; i++) {
        sprintf(servername, baseservname, servers[i]);
        strncpy(sock_addr.sun_path, servername, UNIX_PATH_MAX);
        serversFD[i] = socket(AF_UNIX,SOCK_STREAM,0);
        while (connect(serversFD[i],(struct sockaddr*)&sock_addr, sizeof(sock_addr)) == -1 ) {
            if ( errno == ENOENT ) {
                fprintf(stderr, "ENOENT\n");
                exit(ENOENT);
            } else {
                exit(EXIT_FAILURE);
            }
        }
    }

    TimeSpec* req = malloc(sizeof(TimeSpec));
    atexit_add(req, &varClean, &varCleanCnt);
    TimeSpec* rem = malloc(sizeof(TimeSpec));
    atexit_add(rem, &varClean, &varCleanCnt);
    Int32 nsecs = secret*1.0e6;
    req->tv_sec = nsecs/1.0e9;
    req->tv_nsec = nsecs - (req->tv_sec * 1.0e9);

    int sendingServer = -1;
    for(i=0; i<w; i++) {
        sendingServer = generateRandNumber(p);
        write(serversFD[sendingServer],netIDstring,INT64_DECREP);
        nanosleep(req, rem);
    }

    //Closing connections...
    for(i=0; i<p; i++) {
        close(serversFD[i]);
    }

    printf("CLIENT %llx DONE\n", id);
    fflush(stdout);
    exit(0);
}
