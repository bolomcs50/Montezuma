#include <iostream>
#include <chrono>
#include <thread>
#include <limits.h>
#include <math.h>
#include <cassert>
#include <algorithm>
#include "engine.h"

namespace montezuma
{

    Engine::Engine(std::istream &inputStream, std::ostream &outputStream) : inputStream_(inputStream),
                                                                            outputStream_(outputStream)
    {
        name_ = "Montezuma";
        author_ = "Michele Bolognini";
        evaluatedPositions_ = 0;
    }

    int Engine::protocolLoop()
    {
        std::string command;
        while (true)
        {
            inputStream_ >> command;
            if (command.compare("uci") == 0)
            {
                uciHandShake();
                resetBoard();
            }
            else if (command.compare("isready") == 0)
            {
                // Called once before the GUI asks to calculate a move the first time.
                // Also if the engine is taking time when it is expected to answer, to check if it is alive.
                outputStream_ << "readyok\n";
            }
            else if (command.compare("ucinewgame") == 0)
            {
                resetBoard();
            }
            else if (command.compare("setoption") == 0)
            {
                setOption(inputStream_);
            }
            else if (command.compare("register") == 0)
            {
                // TODO: Find out what registration is and implement it
                outputStream_ << "info string registration is not supported yet\n";
            }
            else if (command.compare("position") == 0)
            {
                std::getline(inputStream_, command);
                updatePosition(command);
            }
            else if (command.find("go", 0) == 0)
            {
                std::getline(inputStream_, command);
                inputGo(command);
            }
            else if (command.find("quit", 0) == 0)
            {
                break;
            }
        }
        return 0;
    }

    // Basic handshake in the UCI protocol
    void Engine::uciHandShake() const
    {
        outputStream_ << "id name " << name_ << "\nid author " << author_ << "\n"
                      << "option name hashSize type spin default 64 min 1 max 128\n"
                      << "option name maxSearchDepth type spin default 6 min 1 max 10\n"
                      << "uciok\n";
    }

    // Reset Board to initial state
    void Engine::resetBoard()
    {
        thc::ChessRules newcr;
        cr_ = newcr;
        isOpening_ = true;
    }

    // plays the moves contained in the string command on the board
    void Engine::updatePosition(const std::string command)
    {
        if (command.find("startpos", 1) == 1)
        {
            resetBoard();
        }
        else if (command.find("fen", 1) == 1)
        {
            resetBoard();
            bool ok = cr_.Forsyth(command.substr(5).c_str());
        }
        std::size_t found = command.find("moves ");
        if (found != std::string::npos)
        { // If moves are specified, play them on the board
            std::string movelist = command.substr(found + 6);
            thc::Move mv;
            size_t start;
            size_t end = 0;
            while ((start = movelist.find_first_not_of(" ", end)) != std::string::npos)
            {
                end = movelist.find(" ", start);
                mv.TerseIn(&cr_, movelist.substr(start, end - start).c_str());
                cr_.PlayMove(mv);
            }
        }
    }

    // Start move evaluation
    void Engine::inputGo(const std::string command)
    {
        std::thread searchThread(&Engine::startSearching, this, command);
        searchThread.detach();
    }

    void Engine::startSearching(const std::string command)
    {
        // Save available time
        unsigned int maxSearchDepth = maxSearchDepth_;
        bool usingTime = false;
        size_t pos = command.find("wtime");
        if (pos != std::string::npos)
        {
            usingTime = true;
            wTime_ = std::stoi(command.substr(pos + 6, command.find_first_of(" ", pos + 6) - pos + 6));
            pos = command.find("btime"); // Supposing that, if wtime is given, btime is given too in the same string
            bTime_ = std::stoi(command.substr(pos + 6, command.find_first_of(" ", pos + 6) - pos + 6));
        }
        unsigned long long int myTime = (cr_.white) ? wTime_ : bTime_;
        // Save depth limit
        pos = command.find("depth");
        if (pos != std::string::npos)
            maxSearchDepth = std::stoi(command.substr(pos + 6, command.find_first_of(" ", pos + 6) - pos + 6));
        int moveHorizon = 50;
        int movesToGo = 0;
        pos = command.find("movestogo");
        if (pos != std::string::npos)
            movesToGo = std::stoi(command.substr(pos + 10, command.find_first_of(" ", pos + 10) - pos + 10));
        // decide how much time to allocate
        unsigned long int limitTime = (movesToGo) ? myTime / std::min(moveHorizon, movesToGo) : myTime / moveHorizon;
        // logFile << "[MONTEZUMA]: I have " << myTime << ", allocated " << limitTime << " to this move." << std::endl;

        line pvLine;
        usingPreviousLine_ = false;
        auto startTimeSearch = std::chrono::high_resolution_clock::now();
        for (int incrementalDepth = 1; incrementalDepth <= maxSearchDepth; incrementalDepth++)
        {
            evaluatedPositions_ = 0;
            auto startTimeThisDepth = std::chrono::high_resolution_clock::now();
            int bestScore = alphaBeta(-MATE_SCORE, MATE_SCORE, incrementalDepth, &pvLine, incrementalDepth); // to avoid overflow when changing sign in recursive calls, do not use INT_MIN as either alpha or beta

            auto stopTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTimeThisDepth);
            auto nps = (duration.count() > 0) ? 1000 * evaluatedPositions_ / duration.count() : 0;
            // Check if the returned score signifies a mate and in how many moves
            if (MATE_SCORE - abs(bestScore) < 100)
            {
                int movesToMate = (bestScore > 0) ? (MATE_SCORE - abs(bestScore) + 1) / 2 : -(MATE_SCORE - abs(bestScore)) / 2;
                outputStream_ << "info score mate " << movesToMate;
            }
            else
            {
                outputStream_ << "info score cp " << bestScore;
            }
            outputStream_ << " depth " << incrementalDepth << " time " << duration.count() << " nps " << nps << " pv ";
            outputStream_ << std::endl;
            usingPreviousLine_ = true;

            // Check if time is up
            stopTime = std::chrono::high_resolution_clock::now();
            auto searchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTimeSearch);
            if (usingTime && searchDuration.count() > limitTime)
                break;
        }

        outputStream_ << "bestmove " << pvLine.moves[0].TerseOut() << std::endl;
        outputStream_.flush();
    }

    int Engine::alphaBeta(int alpha, int beta, int depth, line *pvLine, int initialDepth)
    {
        int score;

        // Base case
        std::vector<thc::Move> legalMoves;
        line line;
        cr_.GenLegalMoveList(legalMoves);

        if (depth == 0 || legalMoves.size() == 0)
        {
            pvLine->moveCount = 0;
            score = evaluate();
            evaluatedPositions_++;
            thc::Move mv;
            mv.dst = thc::SQUARE_INVALID;
            mv.src = thc::SQUARE_INVALID;
            return score;
        }

        thc::Move bestMove = legalMoves[0];
        Flag flag = Flag::ALPHA;
        int moveDepth = initialDepth - depth; // Number of plies played from root position
        if (usingPreviousLine_ && moveDepth < globalPvLine_.moveCount)
        {
            // check that the move is legal. If it is, move it to the top.
            for (int i = 0; i < legalMoves.size(); i++)
                if (legalMoves[i].TerseOut() == globalPvLine_.moves[moveDepth].TerseOut())
                {
                    std::swap(legalMoves[i], legalMoves.front());
                    break;
                }
        }
        else
        {
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
        for (auto mv : legalMoves)
        {
            cr_.PushMove(mv);
            currentScore = -alphaBeta(-beta, -alpha, depth - 1, &line, initialDepth);
            cr_.PopMove(mv);

            // Apply mate score correction (reserve the last 100 points for that)
            if (MATE_SCORE - abs(currentScore) < 100)
            {
                if (currentScore > 0)
                    currentScore--;
                else
                    currentScore++;
            }

            if (currentScore >= beta)
            {
                /* The opponent will not allow this move, he has at least one better choice,
                therefore stop looking for other moves and a precise score: return the upper bound as score approximation,
                since my opponent does at least as good as that here. */
                return beta;
            }
            if (currentScore > alpha)
            { // This move results in a higher minimum guaranteed score: make it new best. Implicitly, this is also < beta.
                alpha = currentScore;
                pvLine->moves[0] = mv;
                memcpy(pvLine->moves + 1, line.moves, line.moveCount * sizeof(thc::Move));
                pvLine->moveCount = line.moveCount + 1;
                usingPreviousLine_ = false;
                bestMove = mv;
                flag = Flag::EXACT;
            }
        }
        return alpha;
    }

    int Engine::evaluate()
    {
        int evalMat{0}, evalPos{0};
        thc::DRAWTYPE drawType;
        if (cr_.IsDraw(cr_.white, drawType))
        { // Check for threefold repetition explicitly
            return 0;
        }

        thc::TERMINAL terminalScore;
        cr_.Evaluate(terminalScore); // Evaluates if position is legal, and if it is terminal
        if (terminalScore == thc::TERMINAL::TERMINAL_WCHECKMATE)
        { // White is checkmated
            if (!cr_.white)
                return MATE_SCORE;
            return -MATE_SCORE;
        }
        else if (terminalScore == thc::TERMINAL::TERMINAL_BCHECKMATE)
        { // Black is checkmated
            if (!cr_.white)
                return -MATE_SCORE;
            return MATE_SCORE;
        }
        else if (terminalScore == thc::TERMINAL::TERMINAL_WSTALEMATE || terminalScore == thc::TERMINAL::TERMINAL_BSTALEMATE)
            return 0;

        else
        {
            cr_.EvaluateLeaf(evalMat, evalPos);
            if (cr_.white)
                return 4 * evalMat + evalPos; // Change sign to eval if its from black side
            return -4 * evalMat + evalPos;
        }
    }

    void Engine::setOption(std::istream &commandStream)
    {
        std::string optionName, optionValue;
        commandStream >> optionName >> optionName; // Throw away the "name" and "value" keys
        commandStream >> optionValue >> optionValue;
        std::cout << "info string setting " << optionName << " to " << optionValue << std::endl;

        if (optionName.compare("maxSearchDepth") == 0)
        {
            maxSearchDepth_ = std::stoi(optionValue);
        }
    }
} // end namespace montezuma