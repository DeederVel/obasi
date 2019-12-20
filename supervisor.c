/*
    Let's roll cause it's a summer apocalypse
*/
#include "obasi_const.h"
#include "commons.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <pthread.h>

typedef struct hashel {
    Int64 elemID;
    int occ;
    int best;
} HashElement;
HashElement** clients = NULL;
int hashSize;

void **varClean = NULL;
int varCleanCnt = 0;

int* children = NULL;
int k;

void *printInfo(void *vargp) {
    FILE* idstdfd = ((FILE*) vargp);

    sigset_t maskInfo;
    sigset_t maskInfoPrec;
    sigfillset(&maskInfo);
    sigprocmask(SIG_SETMASK, &maskInfo, &maskInfoPrec);
    if(clients != NULL) {
        int ci;
        for(ci = 0; ci<hashSize; ci++) {
            if(clients[ci] != NULL) {
                fprintf(
                    idstdfd,
                    "SUPERVISOR ESTIMATE %d FOR %016llx BASED ON %d\n",
                    (clients[ci])->best,
                    (clients[ci])->elemID,
                    (clients[ci])->occ
                );
                fflush(stdout);
            }
        }
    }
    sigprocmask(SIG_SETMASK, &maskInfoPrec, NULL);
    return NULL;
}

void cleanup() {
    int i;
    for(i = 0; i<hashSize; i++) {
        if(clients[i] != NULL) {
            free(clients[i]);
        }
    }
    for(i = 0; i<varCleanCnt; i++) {
        free(varClean[i]);
    }
    free(varClean);
}
TimeSpec* yolo = NULL;
void gestore() {
    pthread_t infoThread;
    printf("\n");
    fflush(stdout);
    if (yolo == NULL) {
        yolo = malloc(sizeof(TimeSpec));
        atexit_add(yolo, &varClean, &varCleanCnt);
        clock_gettime(CLOCK_MONOTONIC, yolo);
        pthread_create(&infoThread, NULL, printInfo, (void*) stderr);
        pthread_detach(infoThread);
    } else {
        TimeSpec* tempSig = malloc(sizeof(TimeSpec));
        atexit_add(tempSig, &varClean, &varCleanCnt);
        clock_gettime(CLOCK_MONOTONIC, tempSig);
        if(difftimeObasi(tempSig, yolo) < 1000) {
            int gi = 0;
            printInfo((void*) stdout);
            fflush(stdout);
            fprintf(stdout, "SUPERVISOR EXITING\n");
            fflush(stdout);
            for(gi = 0; gi < k; gi++) {
                kill(children[gi], SIGUSR1);
            }
            cleanup();
            exit(0);
        } else {
            pthread_create(&infoThread, NULL, printInfo, (void*) stderr);
            pthread_detach(infoThread);
        }
        *yolo = *tempSig;
    }
}

int hashfun(int k, int i, int size) {
    return ((k >> 8) + i) % size;
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("Sintassi avvio: ./supervisor numero-server-da-lanciare\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    int i;
    k = atoi(argv[1]);
    printf("SUPERVISOR STARTING %d\n", k);
    fflush(stdout);

    int* pipeFDs = calloc(k, sizeof(int));
    atexit_add(pipeFDs, &varClean, &varCleanCnt);
    int* maxpipeFD = malloc(sizeof(int));
    atexit_add(maxpipeFD, &varClean, &varCleanCnt);
    *maxpipeFD = 0;
    children = malloc(k*sizeof(int));
    atexit_add(children, &varClean, &varCleanCnt);

    for(i = 0; i<k; i++) {
        int pid;
        char serverID[32];
        char pipeIDR[32];
        char pipeIDW[32];
        int pipeFD[2];

        pipe(pipeFD);
        if(pipeFD[0] > *maxpipeFD) *maxpipeFD = pipeFD[0];
        pipeFDs[i] = pipeFD[0];
        pid = fork();
        if(pid == 0) {
            sprintf(serverID, "%d", (i+1));
            sprintf(pipeIDR, "%d", pipeFD[0]);
            sprintf(pipeIDW, "%d", pipeFD[1]);
            fflush(stdout);
            execl("server", "server", serverID, pipeIDR, pipeIDW, (char*) NULL);
            printf("ERRNO %d\n", errno);
            fflush(stdout);
            exit(0);
        } else {
            children[i] = pid;
        }
    }

    //Gestisco il SIGINT dopo aver lanciato i server
    SigAct sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = gestore;
    sigaction(SIGINT,&sigact,NULL);

    fd_set fds;
    hashSize = 128*k;
    clients = calloc(hashSize, hashSize*sizeof(HashElement));
    atexit_add(clients, &varClean, &varCleanCnt);
    char* bufFD = malloc(sizeof(char)*HYPERBUF_LEN);
    atexit_add(bufFD, &varClean, &varCleanCnt);
    while(TRUE) {
        FD_ZERO(&fds);
        for(i = 0; i<k; i++) {
            FD_SET(pipeFDs[i], &fds);
        }
        select(*maxpipeFD+1, &fds, NULL, NULL, NULL);
        for(i = 0; i<k; i++) {
            if(FD_ISSET(pipeFDs[i], &fds)) {
                if(read(pipeFDs[i], bufFD, HYPERBUF_LEN) > 16) {
                    int clientSec;
                    Int64 clientID;
                    int cell;
                    char* saveTok;
                    char* tokA;
                    char* tokB;
                    tokA = strtok_r(bufFD, ";", &saveTok);
                    tokB = strtok_r(NULL, ";", &saveTok);
                    clientID = strtoll(tokA, NULL, 16);
                    clientSec = atoi(tokB);
                    if(clientID > 0) {
                        printf("SUPERVISOR ESTIMATE %d FOR %016llx FROM %d\n", clientSec, clientID, i);
                        fflush(stdout);
                        int j;
                        for(j=0; j<hashSize; j++) {
                            cell = hashfun(clientID, j, hashSize);
                            if((clients[cell]) == NULL) {
                                (clients[cell]) = malloc(sizeof(HashElement));
                                (clients[cell])->elemID = clientID;
                                (clients[cell])->occ = 1;
                                (clients[cell])->best = clientSec;
                                break;
                            } else {
                                if((clients[cell])->elemID == clientID) {
                                    ((clients[cell])->occ)++;
                                    if(clientSec > 0 && ((clients[cell])->best == -1 || (clients[cell])->best > clientSec) ) {
                                        (clients[cell])->best = clientSec;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
