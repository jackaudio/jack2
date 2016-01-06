#!/bin/bash

rsync -ai --exclude=usr/share /home/coopera/Developer/gear/out/sdkbuild/install/target/jack2/ /home/coopera/Developer/gear/out/sdkbuild/staging/target/
rsync -ai --exclude=usr/share --exclude='*.sym' --exclude='*.h' /home/coopera/Developer/gear/out/sdkbuild/install/target/jack2/ /mnt/gear/
