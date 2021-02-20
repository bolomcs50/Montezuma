#include <iostream>
#include <fstream>
#include <vector>

int main(){
    std::string line;
    std::ifstream resFile;
    resFile.open("results.txt");
    std::string delimiter = " ";
    long long timeTaken{0}, leavesEvaluated{0};
    std::string token;
    std::vector<std::string> words;
    int totalPositions{0};
    
    while (!resFile.eof())
    {
        words.clear();
        std::getline(resFile, line);
        size_t pos = 0;
        while ((pos = line.find(delimiter)) != std::string::npos) {
            token = line.substr(0, pos);
            words.push_back(token);
            line.erase(0, pos + delimiter.length());
        }
        words.push_back(line);
        if (words[0] != "info" && words.size() > 0) continue;
        totalPositions++;
        timeTaken += std::stoi(words[7]);
        leavesEvaluated += std::stoi(words[9]);
    }
    resFile.close();
    printf("\n-> Solved %d positions, %.2f seconds taken and %d leaves evaluated\n", totalPositions, static_cast<float>(timeTaken)/1000, leavesEvaluated);
    printf("Average %d evals per second\n", static_cast<int>(static_cast<float>(leavesEvaluated)/timeTaken*1000));
    return 0;
}
