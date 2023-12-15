//
//  my_utilities.hpp
//  cpp-search-server
//
//  Created by Pavel Sh on 15.12.2023.
//

#ifndef my_utilities_h
#define my_utilities_h
using std::vector;
using std::set;
using std::map;
using std::string;

using std::cout;
using std::cerr;
using std::endl;
using std::operator""s;

//===================Print Range =====================//
template <typename It>
void PrintRange(It start, It finish) {
    while(start != finish) {
        cout << *start << ' ';
        ++start;
    }
    cout << std::endl;
}

//===================Print Range to String=====================//


//====== Helper functions and structs: =======
inline string ReadLine() {
    string s;
    getline(std::cin, s);
    return s;
}

inline int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

#endif /* my_utilities_h */
