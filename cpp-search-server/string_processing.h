#pragma once
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "document.h"

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

//=================== Print Range =====================//
template <typename It>
void PrintRange(It start, It finish) {
    while(start != finish) {
        std::cout << *start << ' ';
        ++start;
    }
    std::cout << std::endl;
}

//=================== Print Match =====================//
void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);
