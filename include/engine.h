#ifndef ENGINE_H
#define ENGINE_H

#include <fstream>
#include "thc.h"
#include "hashing.h"
#include "book.h"

namespace montezuma {

#define MOVE_MAX 1000
#define MATE_SCORE 100000

struct line {
    int moveCount;              // Number of moves in the line.
    thc::Move moves[MOVE_MAX];  // The line.
};

class Engine{
    public:
    /* Constructor */
    Engine();
    /* Effectively starts the engine, listening for command */
    int protocolLoop();
    
    private:
    void uciHandShake() const;
    void displayPosition( thc::ChessRules &cr, const std::string &description) const;
    /* Re-initializes the cr thc::ChessEvaluation board */
    void resetBoard();
    /* Initializes hashTable by setting all elements to 0 */
    void initHashTable();
    /* Plays the listed moves on the Engine's board */
    void updatePosition(const std::string command);
    /* Called when Engine receives the "go" command */
    void inputGo(const std::string command);
    /* Search function */
    int alphaBeta(int alpha, int beta, int depth, line * pvLine, int initialDepth);
    /* Evaluation function, evaluates the engine's current board */
    int evaluate();
    /* Probes the table to see if "hash" is in it. If it is AND the score is useful, return true and its score */
    bool probeHash(int depth, int alpha, int beta, int &score);
    /* Record the hash into the table. Implement replacement scheme here */
    void recordHash(int depth, Flag flag, int score, thc::Move bestMove);
    /* Perform some debugging tasks */
    void debug(const std::string command);
    /* recursively retrieve the PV line using information stored in the table*/
    void retrievePvLineFromTable(line * pvLine);

    thc::ChessEvaluation cr_;
    uint64_t currentHash_;  // Hash of the current position
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
    unsigned int wTime_;
    unsigned int bTime_;
    std::ofstream logFile_;
    std::vector<polyBookEntry> book;

};
} // end namespace montezuma
#endif //ENGINE_H
