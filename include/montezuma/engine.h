#ifndef ENGINE_H
#define ENGINE_H

#include <fstream>
#include <iostream>
#include <set>
#include "thc.h"

enum class Flag {
    NONE,
    EXACT,
    ALPHA,
    BETA
};

namespace montezuma
{

#define MOVE_MAX 1000
#define MATE_SCORE 100000

    struct line
    {
        int moveCount{0};          // Number of moves in the line.
        thc::Move moves[MOVE_MAX]; // The line.
    };

    class Engine
    {
    public:
        /* Constructor */
        Engine(std::istream &inputStream = std::cin, std::ostream &outputStream = std::cout);
        /* Effectively starts the engine, listening for command */
        int protocolLoop();

    private:
        void uciHandShake() const;
        /* Re-initializes the cr thc::ChessEvaluation board */
        void resetBoard();
        /* Plays the listed moves on the Engine's board */
        void updatePosition(const std::string command);
        /* Called when Engine receives the "go" command */
        void inputGo(const std::string command);
        void startSearching(const std::string command);
        /* Search function */
        int alphaBeta(int alpha, int beta, int depth, line *pvLine, int initialDepth);
        /* Evaluation function, evaluates the engine's current board */
        int evaluate();
        void setOption(std::istream &commandStream);

        thc::ChessEvaluation cr_;
        std::string name_;
        std::string author_;
        unsigned long long evaluatedPositions_;
        unsigned int numPositions_;
        unsigned int tableHits_;
        unsigned int tableEntries_;
        line globalPvLine_;
        bool usingPreviousLine_;
        unsigned int wTime_;
        unsigned int bTime_;
        unsigned int maxSearchDepth_{6};
        std::ofstream logFile_;
        bool isOpening_;
        std::istream &inputStream_;
        std::ostream &outputStream_;
    };
} // end namespace montezuma
#endif // ENGINE_H
