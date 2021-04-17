#include <iostream>
#include <string>
#include <bits/stdc++.h>
#include <chrono>
#include <thread>
#include "thc.h"
#include "engine.h"

#define MATE_SCORE 1000000

Engine::Engine(){
    Engine::name = "Montezuma";
    author = "Michele Bolognini";
    nodes = 0;
    hashTableSize = 640; // 64 MB default
}

int Engine::protocolLoop(){
    std::string command;
    while(true){
        std::getline(std::cin, command);
        if (command.compare("uci") == 0){
            uciHandShake();
        } else if (command.compare("isready") == 0){
            // Called once before the GUI asks to calculate a move the first time.
            // Also if the engine is taking time when it is expected to answer, to check if it is alive.
            std::cout << "readyok\n";
        } else if (command.compare("ucinewgame") == 0){
            resetBoard();
            initHashTable();
        } else if (command.find("debug", 0) == 0){
            //TODO
        } else if (command.find("setoption", 0) == 0){
            // This is called to set the internal options of the engine.
            // It is called once per option with the syntax:
            // "setoption name Style value Risky\n"
            // TODO: implement option setting
            std::cout << "info string setoption command is not supported yet\n";
        } else if (command.find("register", 0) == 0){
            //TODO: Find out wtf registration is and implement it
            std::cout << "info string registration is not supported yet\n";
        } else if (command.find("position", 0) == 0){
            updatePosition(command);
        } else if (command.find("go", 0) == 0){
            inputGo(command);
        } else if (command.find("quit", 0) == 0){
            break;
        }
    }
    return 0;
}

// Basic handshake in the UCI protocol
void Engine::uciHandShake() const
{
    std::cout << "id name " << name << "\nid author " << author;
    // TODO: send back 'option' command to tell the GUI which options the engine supports
    std::cout << "\nuciok\n";
}

// Diplays a position to the console
void Engine::displayPosition( thc::ChessRules &cr, const std::string &description) const
{
    std::string fen = cr.ForsythPublish();
    std::string s = cr.ToDebugStr();
    printf( "%s\n", description.c_str() );
    printf( "FEN (Forsyth Edwards Notation) = %s\n", fen.c_str() );
    printf( "Position = %s\n", s.c_str() );
}

// Reset Board to initial state
void Engine::resetBoard(){
    thc::ChessRules newcr;
    cr = newcr;
}

// Resize and empty the hashTable. Do not call this if you don't want to empty the table!
void Engine::initHashTable(){
    numPositions = hashTableSize*0x100000/sizeof(hashEntry); // MB = 1024 KB here.
    hashTable.resize(0);
    hashTable.resize(numPositions);
    tableEntries = 0;
}

// plays the moves contained in the string command on the board
void Engine::updatePosition(const std::string command){
    if (command.find("startpos", 9) == 9){
        resetBoard();
        if (command.find("moves", 18) == 18){    // If moves are specified, play them on the board
            std::string movelist = command.substr(24);
            thc::Move mv;
            std::vector<std::string> moves;
            size_t start;
            size_t end = 0;
            while ((start = movelist.find_first_not_of(" ", end)) != std::string::npos)
            {
                end = movelist.find(" ", start);
                mv.TerseIn(&cr, movelist.substr(start, end - start).c_str());
                cr.PlayMove(mv);
            }                
        }
    } else if (command.find("fen", 9) == 9) {
        bool ok = cr.Forsyth(command.substr(13).c_str());
    }
    currentHash = cr.Hash64Calculate();
}

// Start move evaluation
void Engine::inputGo(const std::string command){
    unsigned int depth = 4;
    std::vector<thc::Move> moves;
    cr.GenLegalMoveListSorted(moves);
    int score{0}, bestScore{INT_MIN};
    // Loop over sorted moves and choose best
    auto startTime = std::chrono::high_resolution_clock::now();
    nodes = 0;
    tableHits = 0;

    // Add current position to history

    for (int d = 1; d <= depth; d++)
    {
        bestScore = alphaBeta(INT_MIN+1, INT_MAX, d, d);     // +1 is NECESSARY to prevent nasty overflow when changing sign. 
    }
    auto stopTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime);
    int nps = (duration.count() > 0) ? 1000*nodes/duration.count() : 0;

    thc::Move pvMove = hashTable[currentHash%numPositions].bestMove;    
    int printScore = bestScore;  
    if (!cr.white) printScore *= -1;

    if (pvMove.TerseOut() != "0000"){
        std::cout << "bestmove " << pvMove.TerseOut() << std::endl;
    }
    
    std::cout << "info score cp " << printScore << " depth " << depth << " time " << duration.count() << " nodes " << nodes << " nps " << nps
              << " tableHits " << tableHits << " tableEntries " << tableEntries << std::endl;

    /* auto tempHash = currentHash;
    do {
        std::cout << pvMove.TerseOut() << " ";
        tempHash = cr.Hash64Update(tempHash, pvMove);
        pvMove = hashTable[tempHash%numPositions].bestMove;
    } while (pvMove.TerseOut() != "0000"); */    
}

int Engine::alphaBeta(int alpha, int beta, int depth, int initialDepth){
    // Base Case
    int score;
    bool moveSearch = false;
    if (cr.GetRepetitionCount() >= 2) return 0;
    // If the table contains the current position and the score is useful, return it
    if (probeHash(depth, alpha, beta, score, moveSearch)){
        tableHits++;
        return score;
    }

    if (depth == 0) {
        nodes++;
        score = evaluate();
        thc::Move bestMove; // Null Move
        recordHash(depth, Flag::EXACT, score, bestMove ); // CHECK if it makes sense to store the leaf instead of evaluating it each time
        return score;
    }

    std::vector<thc::Move> moves;
    cr.GenLegalMoveList(moves);
    if (moves.size() == 0) {
        nodes++;
        return evaluate();
    }

    // If the analysed position is in the table but the result is not definitive, in some cases it can still be useful (see probeHash)
    //if (moveSearch) moves.insert(moves.begin(), hashTable[currentHash%numPositions].bestMove); // Put it first in the vector, when it is found the second time it already is in the table.
    
    // Recursive Step
    Flag flag = Flag::ALPHA;
    //int bestScore = INT_MIN+1;
    thc::Move bestMove;
    for (auto mv:moves){
        cr.PushMove(mv);
        currentHash = cr.Hash64Update(currentHash, mv);
        score = -alphaBeta(-beta, -alpha, depth-1, initialDepth);
        currentHash = cr.Hash64Update(currentHash, mv);
        cr.PopMove(mv);

        // Apply score correction to convey info on distance of mate from current node
        if (score >= MATE_SCORE - initialDepth) score--;
        if (score <= -MATE_SCORE + initialDepth) score++;

        /* FAIL-SOFT IMPLEMENTATION
        if (score > bestScore){ // If this is the best move found, save it
            bestScore = score;
        }
        if (bestScore > alpha){ // if score beats my minimum assured score, it is new best move and update minimum
            alpha = bestScore;
        }
        if (alpha >= beta) {    // If my assured minimum beats the opponent's assured maximum estimated so far, it is the new best
            // Save this as best
            return alpha;
        }
        */
       // FAIL-HARD IMPLEMENTATION
       if (score >= beta){
           recordHash(depth, Flag::BETA, beta, mv);
           return beta; // This move is too good, opponent won't allow it
       }
       if (score > alpha){
           flag = Flag::EXACT;
           bestMove = mv;
           alpha = score; // This move is better than the previous ones, save the score
       }
    }
    recordHash(depth, flag, alpha, bestMove);
    return alpha;
}

int Engine::evaluate(){
    int evalMat{0}, evalPos{0};
    thc::DRAWTYPE drawType;
    if (cr.IsDraw(cr.white, drawType)){
    return 0;
    }

    thc::TERMINAL terminalScore;
    cr.Evaluate(terminalScore);    // Evaluates if position is legal, and if it is terminal
    if( terminalScore == thc::TERMINAL::TERMINAL_WCHECKMATE ){ // White is checkmated
        if (!cr.white) return MATE_SCORE;
        return -MATE_SCORE;
    }
    else if( terminalScore == thc::TERMINAL::TERMINAL_BCHECKMATE ){ // Black is checkmated
        if (!cr.white) return -MATE_SCORE;
        return MATE_SCORE;
    }

    else {
        cr.EvaluateLeaf(evalMat, evalPos);
        if (!cr.white) return -(4*evalMat+evalPos); // Change sign to eval if its from black side
        return 4*evalMat+evalPos;
    }
}

bool Engine::probeHash(int depth, int alpha, int beta, int &score, bool &moveSearch){
    hashEntry *entry = &hashTable[currentHash%numPositions];
    if (entry->key == currentHash){ // Check that the key is the same (not a type ? collision)
        score = entry->score;
        if (entry->depth >= depth){  // If it is useful
            if (entry->flag == Flag::EXACT){
                return true;
            }
            if (entry->flag == Flag::ALPHA && entry->score <= alpha){ // If it was an upper bound and worse than the current one
                score = alpha;
                return true;
            }
            if (entry->flag == Flag::BETA && entry->score >= beta){ // If it was a lower bound and worse than the current one
                score = beta;
                return true;
            }
        } else if (entry->flag == Flag::EXACT || entry->flag == Flag::BETA) moveSearch = true;
        // RememberBestMove()???
    }
    return false;
}
void Engine::recordHash(int depth, Flag flag, int score, thc::Move bestMove){
    hashEntry *entry = &hashTable[currentHash%numPositions];
    
    if (entry->flag == Flag::NONE) tableEntries++; // Count num of occupied cells
    if (entry->flag == Flag::NONE || entry->depth < depth){ // Save the position if there is none in the cell or the depth of the new one is greater
        entry->key = currentHash;
        entry->depth = depth;
        entry->flag = flag;
        entry->score = score;
        entry->bestMove = bestMove;
    }
}