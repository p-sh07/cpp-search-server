#pragma once
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

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"

//===== Server Error Messages =======================
const std::string INVALID_ID_MSG = "SearchServer ERROR: Invalid DocumentID";
const std::string EXISTING_ID_MSG = "SearchServer ERROR: Adding duplicate DocumentID";
const std::string INPUT_INVALID_SYMBOLS_MSG = "SearchServer ERROR: Invalid symbols in input";
const std::string QUERY_WRONG_FORMAT_MSG = "SearchServer ERROR: Incorrect minus-word format used: [-] without word or [--] detected";

//**************** Class Search Server ****************//
class SearchServer {
public:
//====== Constants =================================
    static inline constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;
    static inline constexpr double PRECISION_EPSILON = 1e-6;
//====== Constructors & constructor helpers: =======
    explicit SearchServer(const std::string& stop_words_text);
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    //need this version to avoid checking each word twice in case of a string
    std::set<std::string> ParseStopWordsStr(const std::string& stop_words_text);
    template<typename StringContainer>
    std::set<std::string> ParseStopWords(const StringContainer& stop_words);
//====== Get&Set functions: =========================
    int GetDocumentCount() const;
    int GetDocumentId(int doc_number) const;
    void AddDocument(int document_id, const std::string& document, DocumentStatus status,
                     const std::vector<int>& ratings);
//====== Find Top Documents: ========================
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;
//====== Match Document: ============================
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    
    static inline int ComputeAverageRating(const std::vector<int>& ratings);
    bool IsStopWord(const std::string& word) const;
    std::vector<std::string> ParseStringInput(const std::string& text) const;
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string& word) const;
    
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };
    
    QueryWord ParseQueryWord(std::string text) const;
    
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    
    Query ParseQuery(const std::string& text) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};


//====== Template Definitions: ========================
template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
: stop_words_(ParseStopWords(stop_words)) {}

template<typename StringContainer>
std::set<std::string> SearchServer::ParseStopWords(const StringContainer& stop_words)  {
    std::set<std::string> unique_words = MakeUniqueNonEmptyStrings(stop_words);
    //check all the words are valid:
    for(const auto& word : unique_words) {
        ParseStringInput(word);
    }
    return unique_words;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);
    
    auto matched_documents = FindAllDocuments(query, document_predicate);
    
    std::sort(matched_documents.begin(), matched_documents.end(),
              [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < PRECISION_EPSILON) {
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

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
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
    
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}
