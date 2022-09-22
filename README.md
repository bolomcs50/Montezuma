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
* Transposition tables for faster search and draw detection
* Move ordering
* Support for opening books
* Wiser management of move time
* Implementation of the engine as a state machine with multithreading and pondering
* Implementation of quiescence search to mitigate horizon effect
* Development of a better evaluation function

After implementing those, the engine will be ready for a round of tests against other machines and humans in all time controls.

## Known issues
There is a known issue: given the scope of the thc library the engine is currently based on, draws by repetition cannot be detected during search and evaluation. Relying on hashing would circumvent this problem, but the library does not support Zobrist Hashing (state of the art).
