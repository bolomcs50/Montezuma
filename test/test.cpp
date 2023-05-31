#include "engine.h"
#include <iostream>
#include <sstream> 

int main(int argc, char **argv){
    montezuma::Engine engine;
    std::stringstream commandStream;
    commandStream << "uci\nposition fen r1b2k1r/ppppq3/5N1p/4P2Q/4PP2/1B6/PP5P/n2K2R1 w - - 1 0\ngo\nquit\n";
    std::cout << engine.protocolLoop(commandStream) << std::endl;
    // commandStream << "uci\nposition fen 1rb4r/pkPp3p/1b1P3n/1Q6/N3Pp2/8/P1P3PP/7K w - - 1 0\ngo\nquit\n";
    // std::cout << engine.protocolLoop(commandStream) << std::endl;
    return 0;
}
