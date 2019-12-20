#!/bin/bash
SUPERVISOR_PATH="log/supervisor.log"
SUPERVISOR_POLLS=6
CLIENT_PATH="log/client.log"
CLIENT_COUPLES=10
OOB_PAR_P=5
OOB_PAR_K=8
OOB_PAR_W=20

if [[ ! -d "log/" ]]; then
    mkdir "log"
fi
if [[ -f ${SUPERVISOR_PATH} ]]; then
    unlink ${SUPERVISOR_PATH}
fi
touch ${SUPERVISOR_PATH}
if [[ -f ${CLIENT_PATH} ]]; then
    unlink ${CLIENT_PATH}
fi
touch ${CLIENT_PATH}

echo "Avvio SUPERVISOR"
./supervisor ${OOB_PAR_K}  1>>${SUPERVISOR_PATH} 2>&1 &
sleep 2
echo "Avvio test su 20 client"
printf "Client avviati:    0 su %d\r" "$((CLIENT_COUPLES*2))"
for i in $(seq ${CLIENT_COUPLES}); do
    ./client ${OOB_PAR_P} ${OOB_PAR_K} ${OOB_PAR_W}  1>>${CLIENT_PATH} &
    ./client ${OOB_PAR_P} ${OOB_PAR_K} ${OOB_PAR_W}  1>>${CLIENT_PATH} &
    printf "Client avviati: %4d su %d\r" "$((i*2))" "$((CLIENT_COUPLES*2))"
    sleep 1
done
printf "Richieste dati al supervisor:   0 su %d\r" "${SUPERVISOR_POLLS}"
for j in $(seq ${SUPERVISOR_POLLS}); do
    printf "Richieste dati al supervisor: %3d su %d\r" "$j" "${SUPERVISOR_POLLS}"
    pkill -INT supervisor
    sleep 10
done
printf "\nChiudo SUPERVISOR\n"
trap -- '' SIGINT SIGTERM SIGUSR1
pkill -INT supervisor
pkill -INT supervisor
printf "Elaborazione statistiche\n"
sleep 2
printf "Statistiche:\n"
/bin/bash ./misura.sh ${SUPERVISOR_PATH} ${CLIENT_PATH}
