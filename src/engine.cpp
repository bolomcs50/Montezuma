#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>
#include <limits.h>
#include <math.h>
#include "thc.h"
#include "engine.h"


Engine::Engine(){
    Engine::name = "Montezuma";
    author = "Michele Bolognini";
    nodes = 0;
    hashTableSize = 6400; // 64 MB default
}

int Engine::protocolLoop(){
    std::string command;
    while(true){
        std::getline(std::cin, command);
        logFile.open("Log.txt", std::ios::out | std::ios::app);
        logFile << command << std::endl;
        if (command.compare("uci") == 0){
            uciHandShake();
            resetBoard();
            initHashTable();
        } else if (command.compare("isready") == 0){
            // Called once before the GUI asks to calculate a move the first time.
            // Also if the engine is taking time when it is expected to answer, to check if it is alive.
            std::cout << "readyok\n";
        } else if (command.compare("ucinewgame") == 0){
            resetBoard();
            initHashTable();
        } else if (command.find("debug", 0) == 0){
            debug(command);
        } else if (command.find("setoption", 0) == 0){
            // This is called to set the internal options of the engine.
            // It is called once per option with the syntax:
            // "setoption name Style value Risky\n"
            // TODO: implement option setting
            std::cout << "info string setoption command is not supported yet\n";
        } else if (command.find("register", 0) == 0){
            //TODO: Find out what registration is and implement it
            std::cout << "info string registration is not supported yet\n";
        } else if (command.find("position", 0) == 0){
            updatePosition(command);
        } else if (command.find("go", 0) == 0){
            inputGo(command);
        } else if (command.find("quit", 0) == 0){
            break;
        }
        logFile.close();
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
    } else if (command.find("fen", 9) == 9) {
        bool ok = cr.Forsyth(command.substr(13).c_str());
    }
    std::size_t found = command.find("moves ");
    if (found!=std::string::npos){    // If moves are specified, play them on the board
        std::string movelist = command.substr(found+6);
        thc::Move mv;
        size_t start;
        size_t end = 0;
        while ((start = movelist.find_first_not_of(" ", end)) != std::string::npos)
        {
            end = movelist.find(" ", start);
            mv.TerseIn(&cr, movelist.substr(start, end - start).c_str());
            cr.PlayMove(mv);
        }                
    }
    currentHash = cr.Hash64Calculate();
}

// Start move evaluation
void Engine::inputGo(const std::string command){
    // Save available time
    unsigned int maxSearchDepth = 6;
    size_t pos = command.find("wtime");
    if (pos != std::string::npos){
        //maxSearchDepth = 6;
        wTime = std::stoi(command.substr(pos+6, command.find_first_of(" ", pos+6)-pos+6));
        pos = command.find("btime"); // Supposing that, if wtime is given, btime is given too in the same string
        bTime = std::stoi(command.substr(pos+6, command.find_first_of(" ", pos+6)-pos+6));
    }
    unsigned long long int myTime = (cr.white) ? wTime : bTime;
    // Save depth limit
    pos = command.find("depth");
    if (pos != std::string::npos)
        maxSearchDepth = std::stoi(command.substr(pos+6, command.find_first_of(" ", pos+6)-pos+6));
    int moveHorizon = 50;
    int movesToGo = 0;
    pos = command.find("movestogo");
    if (pos != std::string::npos)
        movesToGo = std::stoi(command.substr(pos+10, command.find_first_of(" ", pos+10)-pos+10));

    // decide how much time to allocate
    unsigned long int limitTime = (movesToGo) ? myTime/std::min(moveHorizon,movesToGo) : myTime/moveHorizon;
    logFile << "[MONTE]: I have " << myTime << ", allocated " << limitTime << " to this move." << std::endl;
    


    // Search
    LINE pvLine;
    usingPreviousLine = false;
    auto startTimeSearch = std::chrono::high_resolution_clock::now();
    for (int incrementalDepth = 1; incrementalDepth <= maxSearchDepth; incrementalDepth++){
        auto startTimeThisDepth = std::chrono::high_resolution_clock::now();
        int bestScore = -alphaBeta(-2*MATE_SCORE, 2*MATE_SCORE, incrementalDepth, &pvLine, incrementalDepth); // to avoid overflow when changing sign in recursive calls, do not use INT_MIN as either alpha or beta
        memcpy(globalPvLine.moves, pvLine.moves, pvLine.moveCount * sizeof(thc::Move));
        globalPvLine.moveCount = pvLine.moveCount;
        auto stopTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTimeThisDepth);
        auto nps = (duration.count() > 0) ? 1000*nodes/duration.count() : 0;
        // Check if the returned score signifies a mate and in how many moves
        if (MATE_SCORE == abs(bestScore)){
            int movesToMate = (pvLine.moveCount+1)/2;
            std::cout << "info score mate " << movesToMate*(2*signbit(bestScore)-1);
        } else {
            std::cout << "info score cp " << bestScore*(2*signbit(bestScore)-1);
        }
        std::cout << " depth " << incrementalDepth << " time " << duration.count() << " nps " << nps << " pv ";
        for (int i=0; i < pvLine.moveCount; i++){
            std::cout << pvLine.moves[i].TerseOut() << " ";
        }
        std::cout << std::endl;
        usingPreviousLine = true;

        // Check if time is up
        stopTime = std::chrono::high_resolution_clock::now();
        auto searchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTimeSearch);
        if (searchDuration.count() > limitTime)
            break;
    }

    std::cout << "bestmove " << pvLine.moves[0].TerseOut() << std::endl;
}

bool toprint = false;

int Engine::alphaBeta(int alpha, int beta, int depth, LINE * pvLine, int initialDepth){
    // Base case
    std::vector<thc::Move> legalMoves;
    LINE line;
    cr.GenLegalMoveList(legalMoves);
    if (depth == 0 || legalMoves.size() == 0){
        pvLine->moveCount = 0;
        return evaluate();
    }
    int moveDepth = initialDepth-depth; // Number of plies played from root position
    if (usingPreviousLine && moveDepth < globalPvLine.moveCount)
        legalMoves.insert(legalMoves.begin(), globalPvLine.moves[moveDepth]);
    /*  Inductive step.
        Alpha = the minimum guaranteed score I can force given my opponent's options. A lower bound, because I can get at least alpha
        Beta = the maximum score my opponent will allow me, given my options. An upper bound, because I cannot my opponent won't let me get more than beta
        As a consequence:
        - I chose the move with highest alpha, trying to maximize it.
        - If a move results in a score > beta, my opponent won't allow it, because he has a better option already.
    */
    for (auto mv:legalMoves){
        cr.PushMove(mv);
        int currentScore = -alphaBeta(-beta, -alpha, depth-1, &line, initialDepth);
        cr.PopMove(mv);
        if (currentScore >= beta){
            /* The opponent will not allow this move, he has at least one better choice,
            therefore stop looking for other moves and a precise score: return the upper bound as score approximation,
            since my opponent does at least as good as that here. */
            return beta;
        }
        if (currentScore > alpha){ // This move results in a higher minimum guaranteed score: make it new best. Implicitly, this is also < beta.
            alpha = currentScore;
            pvLine->moves[0] = mv;
            memcpy(pvLine->moves + 1, line.moves, line.moveCount * sizeof(thc::Move));
            pvLine->moveCount = line.moveCount + 1;
            usingPreviousLine = false;
        }
    }
    return alpha;
}

int Engine::evaluate(){
    int evalMat{0}, evalPos{0};
    thc::DRAWTYPE drawType;
    if (cr.IsDraw(cr.white, drawType)){
        std::cout << "Detected a draw of type " << drawType << std::endl;
        debug("Here");
        return 0;
    }

    thc::TERMINAL terminalScore;
    cr.Evaluate(terminalScore);    // Evaluates if position is legal, and if it is terminal
    if( terminalScore == thc::TERMINAL::TERMINAL_WCHECKMATE ){ // White is checkmated
        if (!cr.white) return MATE_SCORE;
        return -MATE_SCORE;
    } else if( terminalScore == thc::TERMINAL::TERMINAL_BCHECKMATE ){ // Black is checkmated
        if (!cr.white) return -MATE_SCORE;
        return MATE_SCORE;
    } else if (terminalScore == thc::TERMINAL::TERMINAL_WSTALEMATE || terminalScore == thc::TERMINAL::TERMINAL_BSTALEMATE)
        return 0;

    else {
        cr.EvaluateLeaf(evalMat, evalPos);
        if (cr.white) return 4*evalMat+evalPos; // Change sign to eval if its from black side
        return -4*evalMat+evalPos;
    }
}

bool Engine::probeHash(int depth, int alpha, int beta, int &score, bool &moveSearch){
    hashEntry *entry = &hashTable[currentHash%numPositions];
    if (entry->key == currentHash){ // Check that the key is the same (not a type ? collision)
        if (entry->depth >= depth){  // If it is useful
            if (entry->flag == Flag::EXACT){
                score = entry->score;
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

void Engine::debug(const std::string command){
    displayPosition(cr, "Current position is");
}