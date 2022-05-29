# Montezuma
Montezuma Chess Engine. [https://lichess.org/@/Montezuma_BOT](Play me) on lichess.org.
A fairly strong, far from perfect chess player. Decent among humans, weak among computers. For now.
Based off Bill Forster's [https://github.com/billforsternz/thc-chess-library](TripleHappyChess library).

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
The development has hit a roadblock: given the scope of the thc library the engine is currently based on, draws by repetition cannot be detected during search and evaluation. Adding support for hashing would circumvent this problem, but the library does not support Zobrist Hashing (state of the art).
Therefore, Montezuma will probably switch to another library in the near future, before implementing further features.

Among others, these features will be considered for implementation, in no particular order:
* Wiser management of move time
* Implementation of the engine as a state machine with multithreading and pondering
* Implementation of quiescence search to mitigate horizon effect
* Transposition tables for faster search and draw detection
* Support for opening books
* Development of a better evaluation function

After implementing those, the engine will be ready for a round of tests against other machines and humans in all time controls.

