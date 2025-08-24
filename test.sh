#!/bin/bash
set -e
./build.sh -j11 -p -d -n -u
cp ./build/target/desbordante.cpython-312-x86_64-linux-gnu.so ./examples/basic/
source ./venv/bin/activate
python3 ./examples/basic/mining_near.py
deactivate
