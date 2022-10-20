#include <fstream>
#include <iostream>
#include <unistd.h>
#include "book.h"


namespace montezuma {
    
Book::Book(){}
    
bool Book::initialize(std::string fileName){
    std::ifstream bookFile(fileName, std::ios::binary);
    if (!bookFile.is_open()){
        printf("Current working dir: %s\n", get_current_dir_name());
        std::cout << "info string unable to open book " << fileName << std::endl;
        return false;
    }
    // Count number of entries
    std::streampos begin, end;
    begin = bookFile.tellg();
    bookFile.seekg(0, std::ios::end);
    end = bookFile.tellg();
    unsigned long numEntries_ = (end-begin)/sizeof(polyBookEntry);

    // copy over the entries
    bookFile.seekg(0, std::ios::beg);
    positionList_.resize(0);
    positionList_.resize(numEntries_);
    for (int i = 0; i < numEntries_; i++){
        bookFile.read(reinterpret_cast<char*>(&positionList_[i].key), sizeof(positionList_[i].key));
        bookFile.read(reinterpret_cast<char*>(&positionList_[i].move), sizeof(positionList_[i].move));
        bookFile.read(reinterpret_cast<char*>(&positionList_[i].weight), sizeof(positionList_[i].weight));
        bookFile.read(reinterpret_cast<char*>(&positionList_[i].learn), sizeof(positionList_[i].learn));
    }

    std::cout << "info string read " << numEntries_ << " entries from opening book" << std::endl;
    bookFile.close();
    return true;
}

bool Book::listMoves(uint64_t hash, std::vector<polyBookEntry> &moves){
    
    bool found = false;
    for (auto entry:positionList_){
        if (endianSwapU64(entry.key) == hash){
            moves.push_back(entry);
            found = true;  
        }
    }
    return found;
}

bool Book::getMove(thc::ChessEvaluation cr, uint64_t hash, char* moveTerse){
    
    bool found = false;
    std::vector<polyBookEntry> moves;
    // Choose best move and convert it to thc standard
    
    if(listMoves(hash, moves)){
        found = true;
        unsigned short mv = endianSwapU16(moves[0].move);
        moveTerse[0] = ((mv>>6) & 7 )+'a';
        moveTerse[1] = ((mv>>9) & 7 )+1+'0';
        moveTerse[2] = (mv & 7)+'a';
        moveTerse[3] = ((mv>>3) & 7 )+1+'0';
        if (mv>>12){ // If promotion, remember to add it
            char promotionPieces[5] = {'?', 'n', 'b', 'r', 'q'};
            moveTerse[4] = '=';
            moveTerse[5] = promotionPieces[(mv>>12)];
        } else {
            moveTerse[4] = '\0';
        }
        // If using weight, remember to endianSwapU16(entry.weight));
    }
    
    return found;
}

    
unsigned short endianSwapU16(unsigned short x){
    x = (x>>8) | (x<<8);
    return x;
}
unsigned int endianSwapU32(unsigned int x){
    x = (x>>24) | 
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
    return x;
}
uint64_t endianSwapU64 (uint64_t x){
    x = (x>>56) | 
        ((x<<40) & 0x00FF000000000000) |
        ((x<<24) & 0x0000FF0000000000) |
        ((x<<8)  & 0x000000FF00000000) |
        ((x>>8)  & 0x00000000FF000000) |
        ((x>>24) & 0x0000000000FF0000) |
        ((x>>40) & 0x000000000000FF00) |
        (x<<56);
    return x;
}
    
} // end namespace montezuma