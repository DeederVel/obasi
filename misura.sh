#!/bin/bash

declare -A supStats
estimRight=0
estimWrong=0
clientsRead=0
clientIndex=1
jitter=0
shortClients=0

readarray s < $1
for lineS in "${s[@]}"; do
    # SUPERVISOR ESTIMATE 1392 FOR 1c4a3f908bed14a9 BASED ON 2
    #  oppure
    # SUPERVISOR ESTIMATE 2336 FOR 2d80b68d00000000 BASED ON 2
    # ${BASH_REMATCH[1]} -> stima
    # ${BASH_REMATCH[2]} -> ID client
    # ${BASH_REMATCH[3]} -> occorrenze
    #0*(\w{1,8})0{8}

    if [[ ${lineS} =~ SUPERVISOR[[:space:]]ESTIMATE[[:space:]]([[:digit:]]+)[[:space:]]FOR[[:space:]]0*([[:alnum:]]{1,8}?)0{8}[[:space:]]BASED[[:space:]]ON[[:space:]]([[:digit:]]+) ]]; then
        shortClients=1
        supStats[${BASH_REMATCH[2]}]="${BASH_REMATCH[1]},${BASH_REMATCH[3]}"
    else
        if [[ ${lineS} =~ SUPERVISOR[[:space:]]ESTIMATE[[:space:]]([[:digit:]]+)[[:space:]]FOR[[:space:]]0*([[:alnum:]]+)[[:space:]]BASED[[:space:]]ON[[:space:]]([[:digit:]]+) ]]; then
            supStats[${BASH_REMATCH[2]}]="${BASH_REMATCH[1]},${BASH_REMATCH[3]}"
        fi
    fi
done
if [[ $shortClients == 1 ]]; then
    echo -e "\e[1m\t  CLIENT\tREAL\tPROBE\tDELTA\tBASED ON\e[0m"
else
    echo -e "\e[1m\t  CLIENT\t\tREAL\tPROBE\tDELTA\tBASED ON\e[0m"
fi
readarray c < $2
for lineC in "${c[@]}"; do
    # CLIENT 1c4a3f908bed14a9 SECRET 1392
    # ${BASH_REMATCH[1]} -> ID client
    # ${BASH_REMATCH[2]} -> stima
    if [[ ${lineC} =~ CLIENT[[:space:]]([[:alnum:]]+)[[:space:]]SECRET[[:space:]]([[:digit:]]+) ]]; then
        ((clientsRead++))
        temp=${supStats[${BASH_REMATCH[1]}]}
        stima=-1
        basedon=-1
        clientrd=${BASH_REMATCH[2]}
        for kk in ${temp//,/ }; do
            if [[ $stima == -1 ]]; then
                stima=$kk
            else
                basedon=$kk
            fi
        done
        if [ $stima == -1 ]; then
            echo -e "\e[34m${clientIndex}\t• \e[39m${BASH_REMATCH[1]}\t${clientrd}\t\e[1m\e[34mNOT PROBED\e[0m"
        else
            delta=$(($stima - $clientrd))
            if [ $delta -lt 25 ] && [ $delta -gt -25 ]; then
                echo -e "\e[32m${clientIndex}\t• \e[39m${BASH_REMATCH[1]}\t${clientrd}\t${stima}\t\e[1m\e[32m${delta}\e[0m\t${basedon}"
                ((estimRight++))
            else
                echo -e "\e[31m${clientIndex}\t• \e[39m${BASH_REMATCH[1]}\t${clientrd}\t${stima}\t\e[1m\e[31m${delta}\e[0m\t${basedon}"
                ((estimWrong++))
            fi
            ((jitter = jitter + delta))
        fi
        ((clientIndex++))
    fi
done
echo "Correttamente stimati: $estimRight su $clientsRead"
jitterMean=$(($jitter/$clientsRead));
echo "Media errore di stima: ${jitterMean}ms"
