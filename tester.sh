#!/bin/bash
sleep 1
echo "ucinewgame"
cat res/m8n2.txt | grep -E ".*/.*/.*/.*/" | sed 's/\(.*\)/position fen \1\ngo/'
echo "quit"

# sleep 1
# echo "ucinewgame"
# echo "position fen r7/pp1bQ2p/4p1p1/N2N4/3kPB2/P7/1P2BPPP/n4RK1 w - - 5 23"
# echo "go"
# echo "quit"