#include "engine.h"
#include <iostream>
#include <sstream>

int main(int argc, char** argv){
    std::cout << "Here";
    std::stringstream iStream, oStream;
    montezuma::Engine engine(iStream, std::cout);
    std::cout << "Here";
    std::ifstream problemsFile("res/fens2.fen", std::ios::binary);
    std::string fen;
    std::cout << "Here";
    while (getline(problemsFile, fen)){
        return 1;
        std::cout << "Here";
        iStream << "uci\nposition fen" << fen << "\ngo\nquit\n";
        engine.protocolLoop();
        std::cout << oStream.str();
    }
    return 0;
}