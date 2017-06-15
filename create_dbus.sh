#!/bin/bash

if [[ -e "dbus.pid" ]]
then kill `cat dbus.pid`
     rm dbus.pid
fi

raw_session_data=`dbus-daemon --session --print-address --fork --print-pid`

IFS=$'\n' session_data=($raw_session_data)

export DBUS_SESSION_BUS_ADDRESS=${session_data[0]}

echo "${session_data[0]}"
echo "${session_data[0]}" > session.addr
echo "${session_data[1]}"
echo "${session_data[1]}" > dbus.pid
