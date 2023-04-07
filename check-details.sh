#!/bin/bash

echo "Trying server $1"
cd "$HOME/raft-home/raft/$1"

echo "Logs" 
cat log.txt

echo -n "Commit-Index  " 
cat commit-index.txt

echo -n "  Term  "
cat term.txt

echo -n "  Voted-For  "
cat voted-for.txt

echo -n "  Machine-Count  "
cat machine-count.txt

echo ""