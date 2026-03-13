#!/bin/sh

echo "Starting load testing..."

# Requires wrk or ab to be installed.
if ! command -v wrk >/dev/null 2>&1; then
    echo "wrk is required for load testing. Skipping."
    exit 0
fi

echo "Testing Node-style Async Modality"
./build/examples/app_node_style/app_node_style &
PID1=$!
sleep 2
wrk -t4 -c100 -d10s http://127.0.0.1:8080/
kill -9 $PID1

echo "Testing Multithreaded Modality"
./build/examples/app_threaded/app_threaded &
PID2=$!
sleep 2
wrk -t4 -c100 -d10s http://127.0.0.1:8080/
kill -9 $PID2

echo "Testing Multi-process Modality"
./build/examples/app_multi_process/app_multi_process &
PID3=$!
sleep 2
wrk -t4 -c100 -d10s http://127.0.0.1:8080/
kill -9 $PID3

echo "Load testing complete."
