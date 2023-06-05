#include "engine.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("Engine solves mates"){
    std::stringstream commandStream, outputStream;
    montezuma::Engine engine(commandStream, outputStream);
    std::string position, solution, command, attempt;
    commandStream << "uci\n";

    SECTION("Mates in 2"){
        std::ifstream problemsFile("res/MatesIn2.txt", std::ios::in);
        while(getline(problemsFile, position)){
            getline(problemsFile, solution);
            command = "ucinewgame\nposition fen " + position + "\ngo\nquit\n";
            commandStream << command;
            engine.protocolLoop();

            while(getline(outputStream, attempt)){
                if (attempt.find("mate") != std::string::npos){
                    std::cout << "Position: " << position << std::endl << "Attempt: " << attempt << std::endl << "Solution: " << solution << std::endl << std::endl;
                    CHECK(attempt.find(solution) != std::string::npos);
                    outputStream.str(std::string());
                    break;
                }
            }
        }
    }

    SECTION("Mates in 3"){
        std::ifstream problemsFile("res/MatesIn3.txt", std::ios::in);
        while(getline(problemsFile, position)){
            getline(problemsFile, solution);
            command = "ucinewgame\nposition fen " + position + "\ngo\nquit\n";
            commandStream << command;
            engine.protocolLoop();

            while(getline(outputStream, attempt)){
                if (attempt.find("mate") != std::string::npos){
                    std::cout << "Position: " << position << std::endl << "Attempt: " << attempt << std::endl << "Solution: " << solution << std::endl << std::endl;
                    CHECK(attempt.find(solution) != std::string::npos);
                    outputStream.str(std::string());
                    break;
                }
            }
        }
    }
}