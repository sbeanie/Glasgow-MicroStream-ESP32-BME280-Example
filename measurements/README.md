# Measurement Data


## CPU consumption

Using perf

Command:

```
perf record -F 99 -a -g -- ./bme280.rpi
```


## Memory consumption

Using *Working Set Size* tool developed by Brendan Gregg.

Blog: http://www.brendangregg.com/blog/2018-01-17/measure-working-set-size.html
Source code: https://github.com/brendangregg/wss

Command to sampling every 0.1 second:

```
./bme280.rpi
perl wss.pl -C <pid-of-bme280.rpi> 0.1 > output
```
