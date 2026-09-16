#!/bin/bash

set -x

env -i PATH="$PATH" bash -c 'cd EPOS && make veryclean'
env -i PATH="$PATH" bash -c 'cd EPOS && make APPLICATION=RTCSA2026'

riscv64-linux-gnu-objcopy -O binary ./EPOS/img/RTCSA2026.img /srv/tftp/RemoteBootVisionFive2EPOS
