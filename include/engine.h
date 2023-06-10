#ifndef ENGINE_H
#define ENGINE_H

#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <set>
#include "thc.h"
#include "hashing.h"
#include "book.h"

namespace montezuma {

#define MOVE_MAX 1000
#define MATE_SCORE 100000

struct line {
int moveCount{0};               // Number of moves in the line.
    thc::Move moves[MOVE_MAX];  // The line.
};

class Engine{
    public:
    /* Constructor */
    Engine(std::istream& inputStream=std::cin, std::ostream& outputStream=std::cout);
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
    /* Search function */
    int alphaBeta(int alpha, int beta, int depth, int initialDepth);
    /* Evaluation function, evaluates the engine's current board */
    int evaluate();
    /* Probes the table to see if "hash" is in it. If it is AND the score is useful, return true and its score */
    bool probeHash(int depth, int alpha, int beta, int &score);
    /* Record the hash into the table. Implement replacement scheme here */
    void recordHash(int depth, Flag flag, int score, thc::Move bestMove);
    /* recursively retrieve the PV line using information stored in the table*/
    void retrievePvLineFromTable(line * pvLine);
    void retrievePvLineFromTable(line * pvLine, std::set<uint64_t>& hashHistory);
    /* Perform some debugging tasks */
    void debug(const std::string command);
    /* Launch the search, populating the table */
    void search(int maxSearchDepth=MOVE_MAX);
    /* Stop the search and communicate the best move from the pvTable*/
    void giveBestMove(std::chrono::high_resolution_clock::time_point searchStartTime, unsigned long allottedSearchTime);
    

    thc::ChessEvaluation cr_;
    uint64_t currentHash_;  // Hash of the current position
    std::vector<uint64_t> repetitionHashHistory_; // History of the relevant position hashes to check for threefold repetition draws
    std::string name_{"Montezuma"};
    std::string author_{"Michele Bolognini"};
    std::vector<hashEntry> hashTable_;
    unsigned long long evaluatedPositions_{0};
    unsigned int hashTableSize_{64}; // Given in MB
    unsigned int numPositions_{0};
    unsigned int tableHits_{0};
    unsigned int tableEntries_{0};
    line globalPvLine_;
    bool usingPreviousLine_{false};
    unsigned int wTime_{0}, bTime_{0};
    std::ofstream logFile_;
    Book book_;
    bool isOpening_{true};
    bool stopSearch_{false};
    std::istream& inputStream_;
    std::ostream& outputStream_;
    std::vector<std::future<void>> searchThreadsFutures_;

};
} // end namespace montezuma
#endif //ENGINE_H
