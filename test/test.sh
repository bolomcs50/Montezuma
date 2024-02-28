#! /usr/bin/expect

set timeout 10

spawn ../build/montezuma

send "uci\risready\r"
expect "id name Montezuma"
expect "id author Michele Bolognini"
expect "uciok"
expect "readyok"

send "position startpos\rgo\r"
expect "info score cp"
expect "bestmove"

send "ucinewgame\rsetoption name maxSearchDepth value 4\rsetoption name hashSize value 32position startpos\rgo\r"
expect "info score cp"
expect "bestmove"

send "quit\r"
expect eof