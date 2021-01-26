#!/bin/bash
AFL_PRELOAD=/home/sprenger/workspace/uni_aura/remote_control2/firmware/fuzzing/AFL/libdislocator/libdislocator.so

screen -dmS fuzzer1 afl-fuzz -i testcase -o findings_ram -M fuzzer1 -- ../build/fuzzapp @@

MaxCoreCnt=14

counter=2
while [ $counter -le $MaxCoreCnt ]
do
    echo $counter
    screen -dmS fuzzer$counter afl-fuzz -i testcase -o findings_ram -S fuzzer$counter -- ../build/fuzzapp @@ &
    ((counter++))
done

echo finished