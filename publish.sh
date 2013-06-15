#!/bin/bash
# Change the above to the path to your bash, returned from 'which bash', or simply run this script as a parameter to bash.
# This should probably be a python script, since that's what waf is - but - that's left as an exercise for the reader. :)
folder=${PWD##*/}
mkdir -p out
cp src/config.h src/config.bak
cp src/config.EN.Light.Mon.HourVibe src/config.h && ./waf clean build && cp build/$folder.pbw out/Timely.EN.Light.Mon.HourVibe.pbw
cp src/config.EN.Light.Mon.NoVibe   src/config.h && ./waf clean build && cp build/$folder.pbw out/Timely.EN.Light.Mon.NoVibe.pbw
cp src/config.EN.Light.Sun.HourVibe src/config.h && ./waf clean build && cp build/$folder.pbw out/Timely.EN.Light.Sun.HourVibe.pbw
cp src/config.EN.Light.Sun.NoVibe   src/config.h && ./waf clean build && cp build/$folder.pbw out/Timely.EN.Light.Sun.NoVibe.pbw
cp src/config.EN.Dark.Mon.HourVibe  src/config.h && ./waf clean build && cp build/$folder.pbw out/Timely.EN.Dark.Mon.HourVibe.pbw
cp src/config.EN.Dark.Mon.NoVibe    src/config.h && ./waf clean build && cp build/$folder.pbw out/Timely.EN.Dark.Mon.NoVibe.pbw
cp src/config.EN.Dark.Sun.HourVibe  src/config.h && ./waf clean build && cp build/$folder.pbw out/Timely.EN.Dark.Sun.HourVibe.pbw
cp src/config.EN.Dark.Sun.NoVibe    src/config.h && ./waf clean build && cp build/$folder.pbw out/Timely.EN.Dark.Sun.NoVibe.pbw
mv src/config.bak src/config.h
echo ls -l out
ls -l out
