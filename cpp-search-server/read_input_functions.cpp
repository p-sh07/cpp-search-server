#include "read_input_functions.h"

inline int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}

std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}
