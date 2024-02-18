#include "engine.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

TEST_CASE("Engine solves mates")
{
    std::stringstream commandStream, outputStream;
    montezuma::Engine engine(commandStream, outputStream);
    std::string position, solution, command, attempt;
    commandStream << "uci\n";

    SECTION("Mates in 2")
    {
        std::ifstream problemsFile("../res/MatesIn2.txt", std::ios::in);
        int solved = 0; // TODO when the engine is faster, run through the whole set of problems
        while (getline(problemsFile, position) && solved <= 25)
        {
            getline(problemsFile, solution);
            command = "ucinewgame\nposition fen " + position + "\ngo\nquit\n";
            commandStream << command;
            engine.protocolLoop();

            std::string solutionFirstMove;

            while (getline(outputStream, attempt))
            {
                if (attempt.find("mate") != std::string::npos)
                {

                    size_t secondSpace = solution.find(" ", 3);
                    solutionFirstMove = solution.substr(0, secondSpace);

                    std::cout << "Position: " << position << std::endl
                              << "Attempt:\t" << attempt.substr(attempt.find("pv ")) << std::endl
                              << "Solution:\t" << solution << std::endl
                              << std::endl;

                    REQUIRE(attempt.find(solutionFirstMove) != std::string::npos); // require that the first move at least coincides with the solution
                    // TODO require that the solution has the right number of mover
                    outputStream.str(std::string());
                    break;
                }
            }
            solved++;
        }
    }

    SECTION("Mates in 3")
    {
        std::ifstream problemsFile("../res/MatesIn3.txt", std::ios::in);
        int solved = 0; // TODO when the engine is faster, run through the whole set of problems
        while (getline(problemsFile, position) && solved <= 15)
        {
            getline(problemsFile, solution);
            command = "ucinewgame\nposition fen " + position + "\ngo\nquit\n";
            commandStream << command;
            engine.protocolLoop();

            std::string solutionFirstMove;
            while (getline(outputStream, attempt))
            {
                if (attempt.find("mate") != std::string::npos)
                {

                    size_t secondSpace = solution.find(" ", 3);
                    solutionFirstMove = solution.substr(0, secondSpace);

                    std::cout << "Position: " << position << std::endl
                              << "Attempt:\t" << attempt.substr(attempt.find("pv ")) << std::endl
                              << "Solution:\t" << solution << std::endl
                              << std::endl;

                    REQUIRE(attempt.find(solutionFirstMove) != std::string::npos); // require that the first move at least coincides with the solution
                    outputStream.str(std::string());
                    break;
                }
            }
            solved++;
        }
    }
}

TEST_CASE("Performance Benchmarks", "[!benchmark]")
{

    std::stringstream commandStream, outputStream;
    montezuma::Engine engine(commandStream, outputStream);
    std::string command, attempt, token;
    commandStream << "uci\n";

    BENCHMARK_ADVANCED("Mate in 2")
    (Catch::Benchmark::Chronometer meter)
    {
        commandStream << "ucinewgame\nposition fen r3k2r/p3bpp1/2q1p1b1/1ppPP1B1/3n3P/5NR1/PP2NP2/K1QR4 b kq - 0 1\ngo\nquit\n";
        meter.measure([&]
                      { engine.protocolLoop();
                        while (getline(outputStream, attempt)){
                            outputStream >> std::skipws >> token;
                            if (token == "bestmove")
                                return 1;
                        } return 0; });
    };
}