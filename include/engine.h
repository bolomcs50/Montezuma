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
    int negaMax( int depth);
    /* Evaluation function, evaluates the engine's current board */
    int evaluate();
    thc::ChessEvaluation cr;
    std::string name;
    std::string author;
    unsigned long long nodes;
};