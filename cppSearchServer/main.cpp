#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        int total_word_count = 0;
        for(const auto& word : SplitIntoWordsNoStop(document)) {
            //build the inverse index for occurence
            ++word_index_[word][document_id];
            ++total_word_count;
        }
        //upd total word count for the doc (for tf)
        doc_word_data_.push_back(total_word_count);
    }
    
    //v 2. excluding minus words
    vector<Document> FindTopDocuments(const string& raw_query) const {
        set<string> minus_words;
        const set<string> query_words = ParseQuery(raw_query, minus_words);
        auto matched_documents = FindAllDocumentsNoMinus(query_words, minus_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    // doc count = doc_word_data_.size();
    vector<int> doc_word_data_;
    //map <word -> map<docId, count> >
    map<string, map<int, int>> word_index_;
    set<string> stop_words_;
    
    //Inverse Document Frequency (per word per all docs)
    double get_idf(const string& word) const {
        if(word_index_.count(word) == 0)
            return 0;
        //log(totalDocs/number of docs containing the word)
        return log(1.0 * get_total_doc_count()/get_word_in_docs_count(word));
    }
    
    //Term Frequency (per word per document)
    double get_tf(int doc_id, int count) const {
        if(doc_word_data_.size() <= doc_id) {
            throw(std::out_of_range("the doc_id is outside of range stored in SearchServer"));
        } //count in doc / total words in doc :
        return 1.0*count/double(doc_word_data_[doc_id]);
    }
    
    //return total number of stored documents
    int get_total_doc_count() const {
        return int(doc_word_data_.size());
    }
    
    //return number of docs containing word
    int get_word_in_docs_count(const string& word) const {
        if(word_index_.count(word) == 0) {
            return 0;
        }
        return int(word_index_.at(word).size());
    }
    
    //return list of docs containing at least one minus word
    set<int> get_minus_docs(const set<string>& minus_words) const {
        set<int> minus_list;
        for(const auto& word : minus_words) {
            if(word_index_.count(word) != 0) {
                for(const auto [docId, count] : word_index_.at(word))
                minus_list.insert(docId);
            }
        }
        return minus_list;
    }
    
    //upd for minus words
    bool IsStopWord(const string& word) const {
        //(word starts with '=' AND is a stop word) OR is a stop word
        return (word[0] == '-' && stop_words_.count(word.substr(1)) > 0) 
        || stop_words_.count(word) > 0;
    }
    
    //ignores stop words, even with a '-' suffix
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word :  SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    set<string> ParseQuery(const string& text, set<string>& minusWords) const {
        set<string> query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            //found a minus-word
            if(word[0] == '-') { //cut off the -
                minusWords.insert(word.substr(1));
            } else {
                query_words.insert(word);
            }
        }
        return query_words;
    }
  
    vector<Document> FindAllDocumentsNoMinus(const set<string>& query_words, const set<string>& minus_words) const {
        set<int> minus_list = get_minus_docs(minus_words);
        map<int, double> id_relevance;
        //process each query word in turn
        for(const auto& word : query_words) {
            if(word_index_.count(word) != 0) {
                double IDF = get_idf(word); //once per word
                for(const auto& [docId, count] : word_index_.at(word)) {
                    //don't calculate for minus-list docs
                    if(minus_list.count(docId) == 0) {
                        double TF = get_tf(docId, count);
                        id_relevance[docId] += IDF*TF;
                    }
                }
            }
        }
        //transform id_relevance map into a vector of Documents
        vector<Document> matched_documents;
        matched_documents.reserve(id_relevance.size());
        for(const auto&[id, relevance] : id_relevance) {
            matched_documents.push_back({id, relevance});
        }
        return matched_documents;
    }
};


SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}
