#!/bin/bash
sleep 1
echo "ucinewgame"
cat res/m8n2.txt | grep -E ".*/.*/.*/.*/" | sed 's/\(.*\)/position fen \1\ngo/'
echo "quit"