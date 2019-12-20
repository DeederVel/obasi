/*
    Let's roll cause it's a silly day
*/
#include "obasi_const.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include "commons.c"

int SERVERID;
int PIPE_RD;
int PIPE_WR;
int FD_SERV_SOCKET = -1;
char* SERVERNAME = NULL;

void **varClean = NULL;
int varCleanCnt = 0;

void *handle_client(void *vargp) {
    int idfd = *((int*) vargp);
    free((int*) vargp);

    sigset_t sig_block, sig_restore;

    sigemptyset(&sig_block);
    sigaddset(&sig_block, SIGPIPE);

    // Block SIGPIPE for this thread.
    if (pthread_sigmask(SIG_BLOCK, &sig_block, &sig_restore) != 0) {
        printf("SERVER %d Failed to pthread_sigmask \n",idfd);
        fflush(stdout);
        return NULL;
    }

    char buf[INT64_DECREP];
    TimeSpec tstart;
    TimeSpec tfinish;
    int delta = -1;
    int tempDelta = 0;
    Int64 netID = -1;
    tstart.tv_sec = 0;
    tstart.tv_nsec = 0;

    int loopcat = 1;
    while(loopcat == 1) {
        switch(read(idfd,buf,INT64_DECREP)) {
            case 0:
                loopcat = 0;
                break;
            case -1:
                loopcat = 0;
                printf("SERVER %d ERRNO %d", SERVERID, errno);
                fflush(stdout);
                break;
            default:
                clock_gettime(CLOCK_MONOTONIC, &tfinish);
                if( tstart.tv_sec == 0 && tstart.tv_nsec == 0) {
                    //what do
                } else {
                    tempDelta = difftimeObasi(&tfinish, &tstart);
                    if(delta == -1) {
                        delta = tempDelta;
                    } else {
                        if(tempDelta < delta) {
                            delta = tempDelta;
                        }
                    }
                }
                clock_gettime(CLOCK_MONOTONIC, &tstart);
                netID = ntohvl(strtoll(buf, NULL, 10));
                printf("SERVER %d INCOMING %016llx @ %ld.%ld\n", SERVERID, netID, tfinish.tv_sec, tfinish.tv_nsec);
                fflush(stdout);
                break;
        }
    }
    pthread_sigmask(SIG_SETMASK, &sig_restore, NULL);

    char hyperbuf[HYPERBUF_LEN];
    sprintf(hyperbuf, "%016llx;%d", netID, delta);
    printf("SERVER %d CLOSING %016llx ESTIMATE %d\n", SERVERID, netID, delta);
    fflush(stdout);
    if(netID > 0) {
        write(PIPE_WR, hyperbuf, HYPERBUF_LEN);
        printf("SERVER %d SENT %s\n", SERVERID, hyperbuf);
    }
    return NULL;
}

void gestore() {
    exit(0);
}

void cleanup() {
    if(FD_SERV_SOCKET >= 0) {
        close(FD_SERV_SOCKET);
    }
    unlink(SERVERNAME);
    int i;
    for(i = 0; i<varCleanCnt; i++) {
        free(varClean[i]);
    }
    free(varClean);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, SIG_IGN);

    struct sockaddr_un sock_addr;

    SERVERID = atoi(argv[1]);
    PIPE_RD = atoi(argv[2]);
    PIPE_WR = atoi(argv[3]);

    int servnameLen = strlen(SOCK_PATH)+sizeof(int);
    SERVERNAME = malloc(servnameLen*sizeof(char));
    atexit_add(SERVERNAME, &varClean, &varCleanCnt);
    sprintf(SERVERNAME, SOCK_PATH, SERVERID);
    atexit(cleanup);

    SigAct sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = gestore;
    sigaction(SIGUSR1,&sigact,NULL);

    unlink(SERVERNAME);

    strncpy(sock_addr.sun_path, SERVERNAME, UNIX_PATH_MAX);
    sock_addr.sun_family=AF_UNIX;
    FD_SERV_SOCKET=socket(AF_UNIX,SOCK_STREAM,0);
    bind(FD_SERV_SOCKET, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    listen(FD_SERV_SOCKET, MAX_CONNECTIONS);

    printf("SERVER %i ACTIVE\n", SERVERID);
    fflush(stdout);

    while(TRUE) {
        pthread_t thread_id;
        int* newclient = (int*) malloc(sizeof(int));
        *newclient = accept(FD_SERV_SOCKET, NULL, 0);
        printf("SERVER %i CONNECT FROM CLIENT\n", SERVERID);
        fflush(stdout);
        if(*newclient >= 0) {
            pthread_create(&thread_id, NULL, handle_client, (void *) newclient);
            pthread_detach(thread_id);
        }
    }
}
