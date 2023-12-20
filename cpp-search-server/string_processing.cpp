#include "string_processing.h"

using std::cout;
using std::string;
using std::operator""s;

void PrintMatchDocumentResult(int document_id, const std::vector<string>& words, DocumentStatus status) {
    cout << "{ "s
    << "document_id = "s << document_id << ", "s
    << "status = "s << static_cast<int>(status) << ", "s
    << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << std::endl;
}
