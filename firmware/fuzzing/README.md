# Fuzzing 

Fuzzing is the process of testing an application by strategically modifiying some known good input in order 
to find crashes. This process takes a very long time and it is usually recommended to let your application 
be fuzzed for a week. This setup is using AFL (americal fuzzy lop) to test the firmware. 

### Prerequesites

Install *afl++*, *build-essential*, *g++*, *cmake*, *screen*. Navigate into AFL and execute *make* and *sudo make install*. Navigate into *libdislocator* and *make* it aswell.
Copy the path of *libdislocator.so* into *process/startFuzzing.sh* and change up *AFL_PRELOAD*. Also setup the *MaxCoreCnt* variable with your thread count. For multi cpu machines it may be faster to not go all the way with the core count. Check with *afl-whatsup* if the execs/sec increase or decrease with higher counts.


### Running 

Create a folder *findings_ram*. Initialize a ramdisk in it. 

```
sudo mount -t tmpfs none ./findings_ram
```

Be sure to back up findings_ram every so often when the server is not powered through a USP.

Start *process/startFuzzing.sh*. You can stop all instances with *process/stopFuzzing.sh*. Inspect instances with *screen -r fuzzer[Number]*