#include <fstream>
#include "thc.h"
#include "hashTable.h"

#define MATE_SCORE 100000

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
    int alphaBeta(int alpha, int beta, int depth, thc::MOVELIST * pvLine, int initialDepth);
    /* Evaluation function, evaluates the engine's current board */
    int evaluate();
    /* Probes the table to see if "hash" is in it. If it is AND the score is useful, return true and its score */
    bool probeHash(int depth, int alpha, int beta, int &score);
    /* Record the hash into the table. Implement replacement scheme here */
    void recordHash(int depth, Flag flag, int score, thc::Move bestMove);
    /* Perform some debugging tasks */
    void debug(const std::string command);
    /* recursively retrieve the PV line using information stored in the table*/
    void retrievePvLineFromTable(thc::MOVELIST * pvLine);

    thc::ChessEvaluation cr;
    uint64_t currentHash;  // Hash of the current position
    std::string name;
    std::string author;
    unsigned long long nodes;
    std::vector<hashEntry> hashTable;
    unsigned int hashTableSize; // Given in MB
    unsigned int numPositions;
    unsigned int tableHits;
    unsigned int tableEntries;
    thc::MOVELIST globalPvLine;
    bool usingPreviousLine;
    unsigned int wTime;
    unsigned int bTime;
    std::ofstream logFile;

};
