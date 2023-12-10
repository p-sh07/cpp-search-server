//
//  search_server.cpp
//  cpp-search-server
//  Most of SearchServer implementation is in this file (except for template functions)
//
//  Created by Pavel Sh on 10.12.2023.
//


#include "search_server.hpp"

//**************** Class Search Server *************************************//

//====== Constructors & constructor helpers: =======

set<string> SearchServer::ParseStopWordsStr(const string& stop_words_text) {
   const vector<string> all_words = ParseStringInput(stop_words_text);
   return MakeUniqueNonEmptyStrings(all_words);
}


//====== Get&Set functions: =========================

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

int SearchServer::GetDocumentId(int doc_number) const {
    if(doc_number < 0 || doc_number >= documents_.size()) {
        throw std::out_of_range("Document number is out of range!");
    }
    auto nth_document = documents_.begin();
    advance(nth_document, doc_number);
    return nth_document->first; //returns id of nth document
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings)
{
    if(document_id < 0) {
        throw std::invalid_argument("Trying to add Document with negative DocID!");
    }
    const auto words = SplitIntoWordsNoStop(document);
    if((documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Trying to add Document with DocID that already exists in Server!");
    }
    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
}


//====== Find Top Documents: ========================

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                                return document_status == status;
                            });
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}


//====== Match Document: ============================

std::tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);
    if(document_id < 0 || documents_.count(document_id) == 0) {
        throw std::invalid_argument("Invalid Document Id!");
    }
    
    std::tuple<vector<string>, DocumentStatus> result{{}, documents_.at(document_id).status};
    vector<string>& matched_words = std::get<0>(result);
    
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


//====== Private Methods: ============================

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}
///Now only throws on invalid symbols
vector<string> SearchServer::ParseStringInput(const string& text) const {
    vector<string> words;
    string word;
    bool prev_was_minus = false;
    
    for (const char c : text) {
        if(0 <= static_cast<int>(c) && static_cast<int>(c) <= 31){
            throw std::invalid_argument("Invalid symbols in input!");
        } //terminate if invalid symbols detected
        
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            prev_was_minus = false;
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    const auto split_text = ParseStringInput(text);
    for (const string& word : split_text) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

///Query-specific error processing is now in this function
SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    bool is_minus = false;
    //check for incorrect minus positions
    if(text.size() == 1 && text[0] == '-') {
        throw std::invalid_argument("Floating minus [-] in query!");
    } else if (text.size() > 1 && text[0] == '-') {
        if(text[1] == '-') {
            throw std::invalid_argument("Double minus [--] at the start of a word in query!");
        }
        is_minus = true;
        text = text.substr(1);
    }
    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
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
double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
//************ End of Class Search Server *************************************//
