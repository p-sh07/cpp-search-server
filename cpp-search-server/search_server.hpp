//
//  search_server.hpp
//  cpp-search-server
//  Contains some inline helper functions & template functions full definitions
//
//  Created by Pavel Sh on 10.12.2023.
//

#ifndef search_server_h
#define search_server_h

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "helper_functions.hpp"

using std::vector;
using std::set;
using std::map;
using std::string;

using std::ostream;
using std::cerr;
using std::endl;

//====== Helper structs & functions: =======
struct Document {
    Document() = default;
    Document(int id, double relevance, int rating)
    : id(id), relevance(relevance), rating(rating) {}
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}
void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}
ostream& operator<<(ostream& out, const Document& document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s;
    return out;
}


//**************** Class Search Server *************************************//
class SearchServer {
public:
    static inline constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;
    static inline constexpr int PRECISION_EPSILON = 1e-6;
    //====== Default constructor for some tests: =======
    SearchServer() = default;
    
    //====== Constructors & constructor helpers: =======
    explicit SearchServer(const string& stop_words_text)
    : stop_words_(ParseStopWordsStr(stop_words_text)){}
    
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
    : stop_words_(ParseStopWords(stop_words)) {}
    
    //need this version to avoid checking each word twice in case of a string
    set<string> ParseStopWordsStr(const string& stop_words_text);
    
    template<typename StringContainer>
    set<string> ParseStopWords(const StringContainer& stop_words)  {
        set<string> unique_words = MakeUniqueNonEmptyStrings(stop_words);
        //check all the words are valid:
        for(const auto& word : unique_words) {
            ParseStringInput(word);
        }
        return unique_words;
    }
    
    //====== Get&Set functions: =========================
    int GetDocumentCount() const;
    int GetDocumentId(int doc_number) const;
    
    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings);
    
    //====== Find Top Documents: ========================
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);
        
        auto matched_documents = FindAllDocuments(query, document_predicate);
        
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < PRECISION_EPSILON) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const;
    vector<Document> FindTopDocuments(const string& raw_query) const;
    
    //====== Match Document: ============================
    std::tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const;
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    
    static inline int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }
    
    bool IsStopWord(const string& word) const;
    
    vector<string> ParseStringInput(const string& text) const;

    vector<string> SplitIntoWordsNoStop(const string& text) const;
    
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
    
    QueryWord ParseQueryWord(string text) const;
    
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
    
    Query ParseQuery(const string& text) const;
    
    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const;
    
    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                                        {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

//other:

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
                 const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const invalid_argument& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const invalid_argument& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const invalid_argument& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}


#endif /* search_server_h */
