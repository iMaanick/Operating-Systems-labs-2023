#!/bin/bash

pid_file="/var/run/lab1_daemon"

if [[ ! -f "$pid_file" ]]
then
  touch "$pid_file"
fi

chmod 666 "$pid_file"

g++ -Wall -Werror -o lab1_daemon daemon.h daemon.cpp main.cpp
