#include "huffman.hpp"

#include <string>
#include <iostream>


int main(int argc, char **argv) {
    bool v = false;
    bool correct = false;
    std::string command;
    std::string from;
    std::string to;
    if (argc == 5) {
        correct = true;
        if(std::string(argv[1]) == "-v") {
            v = true;
        }
        command = argv[2];
        from = argv[3];
        to = argv[4];
    } else if (argc == 4) {
        correct = true;
        command = argv[1];
        from = argv[2];
        to = argv[3];
    }
    if (command == "-c" && correct) {
        compress(from, to, v);
    } else if (command == "-d" && correct) {
        decompress(from, to, v);
    } else{
        std::cerr << "Use [-v] -(c|d) <from_file_path> <to_file_path>" << std::endl;
        std::cerr << "-c compress" << std::endl;
        std::cerr << "-d decompress" << std::endl;
        std::cerr << "-v print huffman codes" << std::endl;

    }
    return 0;
}

