//
//  search_server.hpp
//  cpp-search-server
//
//  Created by Pavel Sh on 10.12.2023.
//

#ifndef search_server_h
#define search_server_h

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

//for now...
using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

optional<vector<string>> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    bool prev_was_minus = false;
    
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else if(0 <= static_cast<int>(c) &&
                  static_cast<int>(c) <= 31){
            return nullopt;
        } else if(c == '-') {
            if(prev_was_minus) { //error
                return nullopt;
            } else {
                prev_was_minus = true;
                word += c;
            }
        } else {
            prev_was_minus = false;
            word += c;
        }
    }
    if (!prev_was_minus && !word.empty()) {
        words.push_back(word);
    } //catch the trailing -
    if(prev_was_minus) {
        return nullopt;
    }
    
    return words;
}

struct Document {
    Document() = default;
    
    Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
    }
    
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

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

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    // Defines an invalid document id
    // You can refer to this constant as SearchServer::INVALID_DOCUMENT_ID
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    
    //default constructor for testing
    SearchServer() {}
    
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    }
    
    explicit SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text).value())  // Invoke delegating constructor from string container
    {
    }
    
    bool SetStopWords(const string& stop_words_text) {
        auto stopwords_success = SplitIntoWords(stop_words_text);
        if(!stopwords_success) {
            return false;
        }
        stop_words_ = MakeUniqueNonEmptyStrings(stopwords_success.value());
        return true;
    }
    
    [[nodiscard]] bool AddDocument(int document_id, const string& document, DocumentStatus status,
                                   const vector<int>& ratings) {
        if(document_id < 0) {
            return false;
        }
        const auto words = SplitIntoWordsNoStop(document);
        if(!words || (documents_.count(document_id) > 0)) {
            return false;
        }//Error: cannot parse, or id already present
        const double inv_word_count = 1.0 / words->size();
        for (const string& word : words.value()) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        return true;
    }
    
    template <typename DocumentPredicate>
    optional<vector<Document>> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);
        if(!query.has_value()){
            return nullopt;
        }
        auto matched_documents = FindAllDocuments(query.value(), document_predicate);
        
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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
    
    optional<vector<Document>> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                                    return document_status == status;
                                });
    }
    optional<vector<Document>> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }
    
    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }
    int GetDocumentId(int doc_number) const {
        if(doc_number < 0 || doc_number >= documents_.size()) {
            return SearchServer::INVALID_DOCUMENT_ID;
        }
        auto nth_document = documents_.begin();
        advance(nth_document, doc_number);
        return nth_document->first; //returns id of nth document
    }
    
    optional<tuple<vector<string>, DocumentStatus>> MatchDocument(const string& raw_query, int document_id) {
        const auto query = ParseQuery(raw_query);
        if(!query.has_value() || document_id < 0 || documents_.count(document_id) == 0) {
            return nullopt;
        }
        vector<string> matched_words;
        tuple<vector<string>, DocumentStatus> result{{}, documents_.at(document_id).status};
        //loop in minus words
        for (const string& word : query->plus_words) {
            for (const string& word : query->minus_words) {
                if (word_to_document_freqs_.count(word) > 0 && word_to_document_freqs_.at(word).count(document_id)) {
                    return result; //empty, but has value
                }
            }
            if (word_to_document_freqs_.count(word) > 0 && word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        result = {matched_words, documents_.at(document_id).status};
        return result;
    }
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
    
    optional<vector<string>> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        const auto split_text = SplitIntoWords(text);
        if(!split_text) {
            return nullopt;
        }
        for (const string& word : split_text.value()) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
    
    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }
    
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
    
    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }
    
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
    
    optional<Query> ParseQuery(const string& text) const {
        Query query;
        //returns pair bool, vector
        const auto qwords = SplitIntoWords(text);
        if(!qwords) { //some errors in split
            return nullopt; //return error
        }
        for (const string& word : qwords.value()) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }
    
    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    
    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const {
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
