#! /usr/bin/expect

set timeout 20
set input_file [lindex $argv 0]
puts "Reading problems from $input_file"

spawn ../build/montezuma

send "uci\risready\r"
expect "id author Michele"
expect "uciok"
expect "readyok"

set file [open $input_file r]
set solvedProblems 0
while {[gets $file fenLine] != -1} {
    if {[gets $file solLine] != -1} {
        set firstMove [string range $solLine 0 [expr {[string first " " $solLine] - 1}]]
        send "ucinewgame\rposition fen $fenLine\r\go\r"
        expect {
            -re "info score mate.+pv $firstMove" {exp_continue}
            "bestmove $firstMove" {}
            "bestmove " {puts "Wrong answer, expecting $firstMove in FEN $fenLine" ; exit 1}
            timeout {puts "Timed out while waiting: expected move $firstMove FEN $fenLine" ; exit 1}
        }
        incr solvedProblems
    }
    if {$solvedProblems > 40} { #TODO Once the engine is faster, remove problems counter
        exit 0
    }
}

send "quit\r"
expect eof