/* TODO
    - Implement alpha-beta pruning in negamax search
    - Implement/find transposition tables + zobrist hashing for storing evaluations
    - Implement iterative deepening
    - Retrieve PV-line
    - Add support for opening book
    

    - Look into computational time benefits of multithreading
    - Implement multithreading to always have good communicaiton
    - Manage move time
    
    - Write a better eval function

    DONE
    - Add ability to import fen string
    - Implement calculation timing for comparison between methods
    - Consider additional measurements (nodes evaluated, eg)
    - Implement test batteries (e.g. import list of FEN tactics and solve them)
    - Implement proper communication with GUI (points, PV-line, depth, etc.)
*/


#include <iostream>
#include <string>
#include <bits/stdc++.h>
#include <chrono>
#include "thc.h"
#include "engine.h"

Engine::Engine(){
    Engine::name = "Montezuma";
    author = "Michele Bolognini";
    nodes = 0;
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
    hash = cr.Hash64Calculate();
}

// Start move evaluation
void Engine::inputGo(const std::string command){

    unsigned int depth = 4;
    std::vector<thc::Move> moves;
    cr.GenLegalMoveListSorted(moves);
    thc::Move bestMove;
    int score{0}, bestScore{INT_MIN};
    // Loop over sorted moves and choose best
    auto startTime = std::chrono::high_resolution_clock::now();
    nodes = 0;

    bestScore = alphaBeta(INT_MIN+1, INT_MAX, depth);     // +1 is NECESSARY to prevent nasty overflow when changing sign. 

    auto stopTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime);
    int printScore = bestScore;
    if (!cr.white) printScore *= -1;
    std::cout << "bestmove " << bestMove.TerseOut() << std::endl;
    std::cout << "info score cp " << printScore << " depth " << depth << " time "<< duration.count()
        << " nodes " << nodes <<  " nps " << static_cast<int>(1000*nodes/duration.count())
        << " pv " << bestMove.TerseOut() << "\n";
}

int Engine::alphaBeta(int alpha, int beta, int depth){
    // Base Case
    if (depth == 0) {
        nodes++;
        return evaluate();
    }
    std::vector<thc::Move> moves;
    cr.GenLegalMoveList(moves);
    if (moves.size() == 0) {
        nodes++;
        return evaluate();
    }
    
    // Recursive Step
    int score, bestScore = INT_MIN;
    for (auto mv:moves){
        cr.PushMove(mv);
        hash = cr.Hash64Update(hash, mv);
        score = -alphaBeta(-beta, -alpha, depth-1);
        hash = cr.Hash64Update(hash, mv);
        cr.PopMove(mv);
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
       // std::cout << "Evaluation " << score << "\talpha " << alpha << "\tbeta " << beta << "\t depth " << depth << "\n";
       if (score >= beta) return beta; // This move is too good, opponent won't allow it
       if (score > alpha) alpha = score; // This move is better than the previous ones, save the score
    }
    return alpha;
}

int Engine::evaluate(){
    int evalMat{0}, evalPos{0};
    thc::TERMINAL terminalScore;
    cr.Evaluate(terminalScore);    // Evaluates if position is legal, and if it is terminal
    if( terminalScore == thc::TERMINAL::TERMINAL_WCHECKMATE ){ // White is checkmated
        if (!cr.white) return 1000000;
        return -1000000;
    }
    else if( terminalScore == thc::TERMINAL::TERMINAL_BCHECKMATE ){ // Black is checkmated
        if (!cr.white) return -1000000;
        return 1000000;
    }
    else if( terminalScore== thc::TERMINAL::TERMINAL_WSTALEMATE ||
                terminalScore== thc::TERMINAL::TERMINAL_BSTALEMATE )
        return 0;
    else {
        cr.EvaluateLeaf(evalMat, evalPos);
        if (!cr.white) return -(4*evalMat+evalPos); // Change sign to eval if its from black side
        return 4*evalMat+evalPos;
    }
}
