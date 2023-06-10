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

The engine can sometimes beat fairly experienced players at least in selected time controls.
Among others, these features will be considered for implementation, in no particular order:

* Parametrize opening book directory
* Investigate chess.engine.EngineTerminatedError: engine event loop dead errors in lichess-bot
* Communication of more information regarding the search (nodes, hashFull%mill,...)
* Support for tablebase finals
* Wiser management of move time
* Implementation of the engine as a state machine with multithreading and pondering
* Implement full communicaiton with GUIs, enabling all possible options (see the [Protocol Description](http://wbec-ridderkerk.nl/html/UCIProtocol.html))
* Improved move ordering during search
* Implementation of quiescence search to mitigate horizon effect
* Development of a better evaluation function, maybe based on a neural network
* Testing
    * Write unit tests for existing code
* Format code according to some standard
