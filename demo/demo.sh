#!/bin/sh
# A simple shell script that takes pauses between echos
# Lets us see bgtask's asyncronous operation

echo "Starting 1 second task."

sleep 1

echo "Finished 1 second task, starting 10 second task."

sleep 10

echo "Finished 10 second task, starting 2 second task."

sleep 2

echo "All tasks completed."
