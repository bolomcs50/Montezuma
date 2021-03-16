#include "thc.h"

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
    /* Plays the listed moves on the Engine's board */
    void updatePosition(const std::string command);
    /* Called when Engine receives the "go" command */
    void inputGo(const std::string command);
    /* Search function */
    int alphaBeta(int alpha, int beta, int depth);
    /* Evaluation function, evaluates the engine's current board */
    int evaluate();
    thc::ChessEvaluation cr;
    uint64_t hash;
    std::string name;
    std::string author;
    unsigned long long nodes;

};