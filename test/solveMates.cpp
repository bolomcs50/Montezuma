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

            std::string solutionFirstMove;
            while(getline(outputStream, attempt)){
                if (attempt.find("mate") != std::string::npos){

                    size_t secondSpace = solution.find(" ", 3);
                    solutionFirstMove = solution.substr(0, secondSpace);

                    std::cout << "Position: " << position << std::endl
                              << "Attempt:\t" << attempt.substr(attempt.find("pv ")) << std::endl
                              << "Solution:\t" << solution << std::endl << std::endl;

                    REQUIRE(attempt.find("mate 2") != std::string::npos);
                    REQUIRE(attempt.find(solutionFirstMove) != std::string::npos); // require that the first move at least coincides with the solution
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

            std::string solutionFirstMove;
            while(getline(outputStream, attempt)){
                if (attempt.find("mate") != std::string::npos){

                    size_t secondSpace = solution.find(" ", 3);
                    solutionFirstMove = solution.substr(0, secondSpace);

                    std::cout << "Position: " << position << std::endl
                              << "Attempt:\t" << attempt.substr(attempt.find("pv ")) << std::endl
                              << "Solution:\t" << solution << std::endl << std::endl;

                    REQUIRE(attempt.find("mate 3") != std::string::npos);
                    REQUIRE(attempt.find(solutionFirstMove) != std::string::npos); // require that the first move at least coincides with the solution
                    outputStream.str(std::string());
                    break;
                }
            }
        }
    }
}