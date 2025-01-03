#ifndef ENGINE_H
#define ENGINE_H

#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <limits.h>
#include <math.h>
#include <cassert>
#include <algorithm>
#include <set>
#include "thc.h"
#include "hashing.h"
#include "book.h"

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
        void displayPosition(thc::ChessRules &cr, const std::string &description) const;
        /* Re-initializes the cr thc::ChessEvaluation board */
        void resetBoard();
        /* Initializes hashTable by setting all elements to 0 */
        void initHashTable();
        /* Plays the listed moves on the Engine's board */
        void updatePosition(const std::string command);
        /* Called when Engine receives the "go" command */
        void inputGo(const std::string command);
        void startSearching(const std::string command);
        /* Search function */
        int alphaBeta(int alpha, int beta, int depth, line *pvLine, int initialDepth);
        /* Evaluation function, evaluates the engine's current board */
        int evaluate();
        /* Probes the table to see if "hash" is in it. If it is AND the score is useful, return true and its score */
        bool probeHash(int depth, int alpha, int beta, int &score);
        /* Record the hash into the table. Implement replacement scheme here */
        void recordHash(int depth, Flag flag, int score, thc::Move bestMove);
        /* recursively retrieve the PV line using information stored in the table*/
        void retrievePvLineFromTable(line *pvLine);
        void retrievePvLineFromTable(line *pvLine, std::set<uint64_t> &hashHistory);
        /* Checks if the current hash has appeared at least other 2 times in history */
        bool isThreefoldRepetitionHash();
        void setOption(std::istream &commandStream);
        /* Perform some debugging tasks */
        void debug();

        thc::ChessEvaluation cr_;
        uint64_t currentHash_;                        // Hash of the current position
        std::vector<uint64_t> repetitionHashHistory_; // History of the relevant position hashes to check for threefold repetition draws
        std::string name_;
        std::string author_;
        unsigned long long evaluatedPositions_;
        std::vector<hashEntry> hashTable_;
        unsigned int hashTableSize_; // Given in MB
        unsigned int numPositions_;
        unsigned int tableHits_;
        unsigned int tableEntries_;
        line globalPvLine_;
        bool usingPreviousLine_;
        bool usingTime_;
        unsigned long int limitTime_;
        std::chrono::time_point<std::chrono::high_resolution_clock> startTimeSearch_;
        unsigned int wTime_;
        unsigned int bTime_;
        unsigned int maxSearchDepth_{6};
        std::ofstream logFile_;
        Book book_;
        bool isOpening_;
        std::istream &inputStream_;
        std::ostream &outputStream_;
    };
} // end namespace montezuma
#endif // ENGINE_H
