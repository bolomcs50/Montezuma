#include "engine.h"
#include <iostream>
#include <sstream>


int main(int argc, char **argv){
    std::stringstream commandStream;
    montezuma::Engine engine(commandStream, std::cout);
    std::ifstream problemsFile("res/fens2.fen", std::ios::in);
    std::string line, command;
    commandStream << "uci\n";
    while(getline(problemsFile, line)){
        command = "ucinewgame\nposition fen " + line + "\ngo\nquit\n";
        commandStream << command;
        engine.protocolLoop();
        std::cout << command << std::endl;
    }
    return 0;
}