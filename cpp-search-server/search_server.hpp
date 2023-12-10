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

using std::vector;
using std::set;
using std::map;
using std::string;

using std::cerr;
using std::endl;

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
        //Дмитрий, привет! Я считаю, здесь вполне можно было пометить серым ("можно лучше"), а не красным, ибо спорный вопрос)) Да, на две строчки стало короче, но ради одного этого accumulate целый лишний заголовочный файл <numeric> пришлось подключать >_<)
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

#endif /* search_server_h */
