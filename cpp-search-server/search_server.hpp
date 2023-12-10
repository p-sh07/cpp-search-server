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
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std; //for now...

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
    : id(id)
    , relevance(relevance)
    , rating(rating) {
    }
    
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

class SearchServer {
public:
    //default constructor for some tests
    SearchServer() {}
    
    explicit SearchServer(const string& stop_words_text)
    : stop_words_(ParseStopWordsStr(stop_words_text))
    {}
    
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
    : stop_words_(ParseStopWords(stop_words))
    {}
    
    //need this version to avoid checking each word twice in case of a string
    set<string> ParseStopWordsStr(const string& stop_words_text) {
        const vector<string> all_words = ParseStringInput(stop_words_text);
        return MakeUniqueNonEmptyStrings(all_words);
    }
    template<typename StringContainer>
    set<string> ParseStopWords(const StringContainer& stop_words) {
        set<string> unique_words = MakeUniqueNonEmptyStrings(stop_words);
        //check all the words are valid:
        for(const auto& word : unique_words) {
            ParseStringInput(word);
        }
        return unique_words;
    }
    
    void AddDocument(int document_id, const string& document, DocumentStatus status,
                                   const vector<int>& ratings) {
        if(document_id < 0) {
            throw invalid_argument("Trying to add Document with negative DocID!");
        }
        const auto words = SplitIntoWordsNoStop(document);
        if((documents_.count(document_id) > 0)) {
            throw invalid_argument("Trying to add Document with DocID that already exists!");
        }
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }
    
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, document_predicate);
        
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
    
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                                    return document_status == status;
                                });
    }
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }
    
    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }
    int GetDocumentId(int doc_number) const {
        if(doc_number < 0 || doc_number >= documents_.size()) {
            throw out_of_range("Document number is out of range!");
        }
        auto nth_document = documents_.begin();
        advance(nth_document, doc_number);
        return nth_document->first; //returns id of nth document
    }
    
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) {
        const auto query = ParseQuery(raw_query);
        if(document_id < 0 || documents_.count(document_id) == 0) {
            throw invalid_argument("Invalid Document Id!");
        }
        
        tuple<vector<string>, DocumentStatus> result{{}, documents_.at(document_id).status};
        vector<string>& matched_words = get<0>(result);
        
        //loop in minus words first
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id)) {
                return result; //will return empty vector
            }
        } //if no minus words found, loop in plus words:
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
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
    
    ///Переработанная функция SplitIntoWords()
    ///Обработка ошибок парсинга проводится в этой функции, т.к. она проходит по каждому символу ввода(query & document & stop words).
    ///Проходить по всем символам ввода еще раз в отдельной функции было бы излишне.
    vector<string> ParseStringInput(const string& text) const {
        vector<string> words;
        string word;
        bool prev_was_minus = false;
        
        for (const char c : text) {
            if(0 <= static_cast<int>(c) && static_cast<int>(c) <= 31){
                throw invalid_argument("Invalid symbols in input!");
            } //terminate if invalid symbols detected
            
            if (c == ' ') {
                if(prev_was_minus) {
                    throw invalid_argument("Trailing [-] at the end of a word in input!");
                } else if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            } else if(c == '-') {
                if(prev_was_minus) {
                    throw invalid_argument("Double [--] detected in input!");
                } else {
                    prev_was_minus = true;
                    word += c;
                }
            } else {
                prev_was_minus = false;
                word += c;
            }
        }
            //catch the trailing -
        if(prev_was_minus) {
            throw invalid_argument("Trailing [-] in input!");
        } else if (!word.empty()) {
            words.push_back(word);
        }
        return words;
    }

    
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        const auto split_text = ParseStringInput(text);
        for (const string& word : split_text) {
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
    
    Query ParseQuery(const string& text) const {
        Query query;
        //returns pair bool, vector
        const auto qwords = ParseStringInput(text);
        
        for (const string& word : qwords) {
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
