#!/bin/bash

for id in $(ipcs -m | grep ^0x | grep -v ^0x00000000 | cut -d" " -f1); do
   echo "Removing shared memory $id"
   ipcrm -M $id;
done

for id in $(ipcs -s | grep ^0x | grep -v ^0x00000000 | cut -d" " -f1); do
   echo "Removing semaphore $id"
   ipcrm -S $id;
done

