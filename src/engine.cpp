#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <future>
#include <limits.h>
#include <math.h>
#include <cassert>
#include <algorithm>
#include "engine.h"

namespace montezuma {

Engine::Engine(std::istream& inputStream, std::ostream& outputStream):
inputStream_ (inputStream),
outputStream_ (outputStream)
{
    name_ = "Montezuma";
    author_ = "Michele Bolognini";
    evaluatedPositions_ = 0;
    hashTableSize_ = 64; // 64 MB default
    book_.initialize("engines/Human.bin");
//    book_.initialize("res/Titans.bin");
}

int Engine::protocolLoop(){
    std::vector<std::future<void>> futures;
    std::string command;
    while(true){
        std::getline(inputStream_, command);
        if (command.compare("uci") == 0){
            uciHandShake();
            resetBoard();
            initHashTable();
        } else if (command.compare("isready") == 0){
            // Called once before the GUI asks to calculate a move the first time.
            // Also if the engine is taking time when it is expected to answer, to check if it is alive.
            outputStream_ << "readyok\n";
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
            outputStream_ << "info string setoption command is not supported yet\n";
        } else if (command.find("register", 0) == 0){
            //TODO: Find out what registration is and implement it
            outputStream_ << "info string registration is not supported yet\n";
        } else if (command.find("position", 0) == 0){
            updatePosition(command);
        } else if (command.find("go", 0) == 0){
            futures.push_back(std::async(std::launch::async, &Engine::inputGo, this, command));
            // inputGo(command);
        } else if (command.find("quit", 0) == 0){
            break;
        }
    }
    return 0;
}

// Basic handshake in the UCI protocol
void Engine::uciHandShake() const
{
    outputStream_ << "id name " << name_ << "\nid author " << author_;
    // TODO: send back 'option' command to tell the GUI which options the engine supports
    outputStream_ << "\nuciok\n";
}

// Diplays a position to the console
void Engine::displayPosition( thc::ChessRules &cr, const std::string &description) const
{
    std::string fen = cr.ForsythPublish();
    std::string s = cr.ToDebugStr();
    printf( "%s\n", description.c_str() );
    printf( "FEN = %s", fen.c_str() );
    printf( "%s", s.c_str() );
    outputStream_ << "Hash64: " << zobristHash64Calculate(cr) << std::endl << "currentHash: " << currentHash_ << std::endl;
}

// Reset Board to initial state
void Engine::resetBoard(){
    thc::ChessRules newcr;
    cr_ = newcr;
    isOpening_ = true;
    repetitionHashHistory_.clear();
}

// Resize and empty the hashTable. Do not call this if you don't want to empty the table!
void Engine::initHashTable(){
    numPositions_ = hashTableSize_*1024*1024/sizeof(hashEntry); // MB = 1024 KB here.
    hashTable_.clear();
    hashTable_.resize(numPositions_);
    tableEntries_ = 0;
}

// plays the moves contained in the string command on the board
void Engine::updatePosition(const std::string command){
    if (command.find("startpos", 9) == 9){
        resetBoard();
    } else if (command.find("fen", 9) == 9) {
        resetBoard();
        bool ok = cr_.Forsyth(command.substr(13).c_str());
    }
    currentHash_ = zobristHash64Calculate(cr_);
    repetitionHashHistory_.clear();
    repetitionHashHistory_.push_back(currentHash_);
    std::size_t found = command.find("moves ");
    if (found!=std::string::npos){    // If moves are specified, play them on the board
        std::string movelist = command.substr(found+6);
        thc::Move mv;
        size_t start;
        size_t end = 0;
        while ((start = movelist.find_first_not_of(" ", end)) != std::string::npos)
        {
            end = movelist.find(" ", start);
            mv.TerseIn(&cr_, movelist.substr(start, end - start).c_str());
            currentHash_ = zobristHash64Update(currentHash_, cr_, mv);
            if (cr_.squares[mv.dst]!=' ' || cr_.squares[mv.src]=='p' || cr_.squares[mv.src]=='P')
                repetitionHashHistory_.clear();
            else
                repetitionHashHistory_.push_back(currentHash_);
            cr_.PlayMove(mv);
        }        
    }
    for (auto hash:repetitionHashHistory_)
        hashTable_[hash%numPositions_].repetitionCount = std::count(repetitionHashHistory_.begin(), repetitionHashHistory_.end(), hash);
    
}

// Start move evaluation
void Engine::inputGo(const std::string command){
    // Save available time    
    unsigned int maxSearchDepth = 6;
    bool usingTime = false;
    size_t pos = command.find("wtime");
    if (pos != std::string::npos){
        usingTime = true;
        wTime_ = std::stoi(command.substr(pos+6, command.find_first_of(" ", pos+6)-pos+6));
        pos = command.find("btime"); // Supposing that, if wtime is given, btime is given too in the same string
        bTime_ = std::stoi(command.substr(pos+6, command.find_first_of(" ", pos+6)-pos+6));
    }
    unsigned long long int myTime = (cr_.white) ? wTime_ : bTime_;
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
    // logFile << "[MONTEZUMA]: I have " << myTime << ", allocated " << limitTime << " to this move." << std::endl;

    // Search
    // If the position is in the opening book, use it
    char* bestMove = (char*)malloc(6*sizeof(char));
    if (isOpening_ && book_.getMove(cr_, currentHash_, bestMove)){
        outputStream_ << "bestmove " << bestMove << std::endl;
        return;
    } else // Otherwise stop looking in the book
        isOpening_ = false;
    free(bestMove);
    
    line pvLine;
    usingPreviousLine_ = false;
    auto startTimeSearch = std::chrono::high_resolution_clock::now();
    
    for (int incrementalDepth = 1; incrementalDepth <= maxSearchDepth; incrementalDepth++){
        evaluatedPositions_ = 0;
        auto startTimeThisDepth = std::chrono::high_resolution_clock::now();
        int bestScore = alphaBeta(-MATE_SCORE, MATE_SCORE, incrementalDepth, &pvLine, incrementalDepth); // to avoid overflow when changing sign in recursive calls, do not use INT_MIN as either alpha or beta
        globalPvLine_.moveCount = 0;
        retrievePvLineFromTable(&globalPvLine_);
        
        auto stopTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTimeThisDepth);
        auto nps = (duration.count() > 0) ? 1000*evaluatedPositions_/duration.count() : 0;
        // Check if the returned score signifies a mate and in how many moves
        if (MATE_SCORE-abs(bestScore) < 100){
            int movesToMate = (bestScore > 0 ) ? (MATE_SCORE-abs(bestScore)+1)/2 : -(MATE_SCORE-abs(bestScore))/2;
            outputStream_ << "info score mate " <<  movesToMate;
        } else {
            outputStream_ << "info score cp " << bestScore;
        }
        outputStream_ << " depth " << incrementalDepth << " time " << duration.count() << " nps " << nps << " pv ";
        for (int i=0; i < globalPvLine_.moveCount; i++){
            outputStream_ << globalPvLine_.moves[i].TerseOut() << " ";
        }
        outputStream_ << std::endl;
        usingPreviousLine_ = true;

        // Check if time is up
        stopTime = std::chrono::high_resolution_clock::now();
        auto searchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTimeSearch);
        if (usingTime && searchDuration.count() > limitTime)
            break;

    }

    outputStream_ << "bestmove " << globalPvLine_.moves[0].TerseOut() << std::endl;
}

bool toprint = false;

int Engine::alphaBeta(int alpha, int beta, int depth, line * pvLine, int initialDepth){
    int score;
    if (probeHash(depth, alpha, beta, score))
    	return score;
	// Base case
    std::vector<thc::Move> legalMoves;
    line line;
    cr_.GenLegalMoveList(legalMoves);

    if (depth == 0 || legalMoves.size() == 0){
        pvLine->moveCount = 0;
        score = evaluate();
        evaluatedPositions_++;
        thc::Move mv;
        mv.dst = thc::SQUARE_INVALID;
        mv.src = thc::SQUARE_INVALID;
        recordHash(depth, Flag::EXACT, score, mv);
        return score;
    }

    thc::Move bestMove = legalMoves[0];
    Flag flag = Flag::ALPHA;
    int moveDepth = initialDepth-depth; // Number of plies played from root position
    if (usingPreviousLine_ && moveDepth < globalPvLine_.moveCount){
        //check that the move is legal. If it is, move it to the top.
        for (int i = 0; i < legalMoves.size(); i++)
            if (legalMoves[i].TerseOut() == globalPvLine_.moves[moveDepth].TerseOut()){
                std::swap(legalMoves[i], legalMoves.front());
                break;
            }
    } else {
        usingPreviousLine_ = false;
    }
        
    /*  Inductive step.
        Alpha = the minimum guaranteed score I can force given my opponent's options. A lower bound, because I can get at least alpha
        Beta = the maximum score my opponent will allow me, given my options. An upper bound, because my opponent won't let me get more than beta
        As a consequence:
        - I chose the move with highest alpha, trying to maximize it.
        - If a move results in a score > beta, my opponent won't allow it, because he has a better option already.
    */
    int currentScore{0};
    for (auto mv:legalMoves){
        currentHash_ = zobristHash64Update(currentHash_, cr_, mv);
        cr_.PushMove(mv);
        hashTable_[currentHash_%numPositions_].repetitionCount++;
        currentScore = -alphaBeta(-beta, -alpha, depth-1, &line, initialDepth);
        hashTable_[currentHash_%numPositions_].repetitionCount--;
        cr_.PopMove(mv);
        currentHash_ = zobristHash64Update(currentHash_, cr_, mv);       
        
        // Apply mate score correction (reserve the last 100 points for that)
        if (MATE_SCORE-abs(currentScore) < 100 ){
            if (currentScore > 0) currentScore--;
            else currentScore++;
        }
           
        if (currentScore >= beta){
            /* The opponent will not allow this move, he has at least one better choice,
            therefore stop looking for other moves and a precise score: return the upper bound as score approximation,
            since my opponent does at least as good as that here. */
            recordHash(depth, Flag::BETA, beta, mv);
            return beta;
        }
        if (currentScore > alpha){ // This move results in a higher minimum guaranteed score: make it new best. Implicitly, this is also < beta.
            alpha = currentScore;
            pvLine->moves[0] = mv;
            memcpy(pvLine->moves + 1, line.moves, line.moveCount * sizeof(thc::Move));
            pvLine->moveCount = line.moveCount + 1;
            usingPreviousLine_ = false;
            bestMove = mv;
            flag = Flag::EXACT;
        }
    }
    recordHash(depth, flag, alpha, bestMove);
    return alpha;
}

int Engine::evaluate(){
    int evalMat{0}, evalPos{0};
    thc::DRAWTYPE drawType;
    if (cr_.IsDraw(cr_.white, drawType)){ // Check for threefold repetition explicitly
        return 0;
    }
    
    thc::TERMINAL terminalScore;
    cr_.Evaluate(terminalScore);    // Evaluates if position is legal, and if it is terminal
    if( terminalScore == thc::TERMINAL::TERMINAL_WCHECKMATE ){ // White is checkmated
        if (!cr_.white) return MATE_SCORE;
        return -MATE_SCORE;
    } else if( terminalScore == thc::TERMINAL::TERMINAL_BCHECKMATE ){ // Black is checkmated
        if (!cr_.white) return -MATE_SCORE;
        return MATE_SCORE;
    } else if (terminalScore == thc::TERMINAL::TERMINAL_WSTALEMATE || terminalScore == thc::TERMINAL::TERMINAL_BSTALEMATE)
        return 0;

    else {
        cr_.EvaluateLeaf(evalMat, evalPos);
        if (cr_.white) return 4*evalMat+evalPos; // Change sign to eval if its from black side
        return -4*evalMat+evalPos;
    }
}

bool Engine::probeHash(int depth, int alpha, int beta, int &score){
    
    hashEntry *entry = &hashTable_[currentHash_%numPositions_];
    if (entry->key == currentHash_){ // Check that the key is the same (not a type ? collision)
        if (entry->depth >= depth){  // If it was already searched at a depth greater than the one requested now
            if(entry->repetitionCount >= 2){ // If the position has been reached 3 times already, return 0, as it is a draw.
                score = 0;
                entry->score = 0;
                entry->flag = Flag::EXACT;
                return true;
            }
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
        }
    }
    return false;
}

void Engine::recordHash(int depth, Flag flag, int score, thc::Move bestMove){
    hashEntry *entry = &hashTable_[currentHash_%numPositions_];
    if (entry->flag == Flag::NONE) tableEntries_++; // Count num of occupied cells
    if (entry->flag == Flag::NONE || entry->depth <= depth){ // Save the position if there is none in the cell or the depth of the new one is greater
        entry->key = currentHash_;
        entry->depth = depth;
        entry->flag = flag;
        entry->score = score;
        entry->bestMove = bestMove;
    }
    
}

void Engine::retrievePvLineFromTable(line * pvLine) {
    std::set<uint64_t> hashHistory;
    retrievePvLineFromTable(pvLine, hashHistory);
    return;
}

void Engine::retrievePvLineFromTable(line * pvLine, std::set<uint64_t>& hashHistory){
    hashEntry *entry = &hashTable_[currentHash_%numPositions_];
    if (entry->flag == Flag::NONE || entry->bestMove.src >= thc::SQUARE_INVALID || entry->bestMove.dst >= thc::SQUARE_INVALID
        || entry->bestMove.TerseOut() == "0000" || entry->key != currentHash_ || pvLine->moveCount >= 30 || hashHistory.count(currentHash_))
        return;
    
    pvLine->moveCount++;
    pvLine->moves[pvLine->moveCount-1] = entry->bestMove;
    
    hashHistory.insert(currentHash_);
    currentHash_ = zobristHash64Update(currentHash_, cr_, entry->bestMove);
    cr_.PushMove(entry->bestMove);
    retrievePvLineFromTable(pvLine, hashHistory);
    cr_.PopMove(entry->bestMove);
    currentHash_ = zobristHash64Update(currentHash_, cr_, entry->bestMove);
    hashHistory.erase(currentHash_);
}

void Engine::debug(const std::string command){
    displayPosition(cr_, "Current position is");
    printf("Recorded %u hashTableEntries\n", tableEntries_);
    outputStream_ << currentHash_%numPositions_ << std::endl;
    hashEntry *entry = &hashTable_[currentHash_%numPositions_];
    outputStream_ << "Entry at " << currentHash_%numPositions_ << ": ";
    printf("depth:%d, flag:%d, score:%d, repetitions:%u, bestMove:", entry->depth, entry->flag, entry->score, entry->repetitionCount);
    outputStream_ << entry->bestMove.TerseOut() << std::endl;
}

} //end namespace montezuma