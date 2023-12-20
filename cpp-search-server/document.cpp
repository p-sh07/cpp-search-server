#include "document.h"

using std::cout;
using std::endl;
using std::string;
using std::operator""s;

Document::Document(int id, double relevance, int rating)
: id(id), relevance(relevance), rating(rating) {}

void PrintDocument(const Document& document) {
    cout << "{ "s
    << "document_id = "s << document.id << ", "s
    << "relevance = "s << document.relevance << ", "s
    << "rating = "s << document.rating
    << " }"s << endl;
}

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "s
    << "document_id = "s << document.id << ", "s
    << "relevance = "s << document.relevance << ", "s
    << "rating = "s << document.rating
    << " }"s;
    return out;
}
