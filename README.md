# Montezuma
Montezuma Chess Engine. [Play me](https://lichess.org/@/Montezuma_BOT) on lichess.org.
A fairly strong, far from perfect chess player. Decent among humans, weak among computers. For now.
Based off Bill Forster's [TripleHappyChess library](https://github.com/billforsternz/thc-chess-library).

## Installation
Clone the repository
```
git clone https://github.com/bolomcs50/Montezuma/tree/main
```

Compile into a binary using your environment (CMakeLists.txt files are already set up).  
Open your chess GUI of choice, add an engine and select the executable file.  
Free chess GUIs exist, such as:  
* [Banksia GUI](https://banksiagui.com/)
* [Arena GUI](http://www.playwitharena.de/)
* [XBoard](https://www.gnu.org/software/xboard/)

## Current state and Future development
The engine can now beat fairly experienced players at least in selected time controls.

Among others, these features will be considered for implementation, in no particular order:
* Support for opening books
* Draw by repetition check with hashes
* Communication of more information regardin the search (nodes, nps, hashTable full%,...)
* Support for tablebase finals
* refactor: put everything inside a proper namespace
* Wiser management of move time
* Implementation of the engine as a state machine with multithreading and pondering
* Improved move ordering
* Implementation of quiescence search to mitigate horizon effect
* Development of a better evaluation function, maybe based on a neural network

After implementing those, the engine will be ready for a round of tests against other machines and humans in all time controls.

## Known issues
* The engine does not see draws by repetition as such and plays them even if it is winning
* There is one problem in fens3 where it does not get to mate, though it should