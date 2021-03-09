#include "thc.h"

#define hashfEXACT 0
#define hashfALPHA 1
#define hashfBETA  2

enum class Flag {
    NONE,
    EXACT,
    ALPHA,
    BETA
};

struct hashEntry {
    uint64_t key;
    int depth;
    Flag flag;
    int score;
    thc::Move bestMove;
};