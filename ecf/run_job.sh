#!/bin/bash

rm -vf bjob_cray_disk_benchmark.stderr bjob_cray_disk_benchmark.stdout

bsub < cray_disk_benchmark.ecf
