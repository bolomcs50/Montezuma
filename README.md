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
Nevertheless, the development has hit a roadblock: given the scope of the thc library the engine is currently based on, draws by repetition cannot be detected during search and evaluation. Adding support for hashing would circumvent this problem, but the library does not support Zobrist Hashing (state of the art).
Therefore, Montezuma will probably switch to another library (like https://github.com/nkarve/surge) in the near future, before implementing further features.


Among others, these features will be considered for implementation, in no particular order:
* Parametrize opening book directory
* Communication of more information regardin the search (nodes, hashFull%mill,...)
* Support for tablebase finals
* Wiser management of move time
* Implementation of the engine as a state machine with multithreading and pondering
* Improved move ordering
* Implementation of quiescence search to mitigate horizon effect
* Development of a better evaluation function, maybe based on a neural network
* Automate testing
* Implement full communicaiton with GUIs, enabling all possible options (see the [Protocol Description](http://wbec-ridderkerk.nl/html/UCIProtocol.html ))

## Known issues
