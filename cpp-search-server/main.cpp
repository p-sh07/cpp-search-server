#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <map>
#include <set>
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

struct QueryWord {
    QueryWord(string word, bool is_minus = false)
    : str(word), is_minus(is_minus) {}
    string str;
    bool is_minus;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        ++no_of_docs;
        const auto doc_words = SplitIntoWordsNoStop(document);
        //build the inverse index for occurence
        double inverse_total_word_count = 1.0/doc_words.size();
        for(const auto& word : doc_words) {
            word_index_[word][document_id] += inverse_total_word_count;
        }
    }
    
    //v 2. excluding minus words
    vector<Document> FindTopDocuments(const string& raw_query) const {
        set<string> minus_words;
        const vector<QueryWord> query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocumentsNoMinus(query_words);

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
    int no_of_docs = 0;
    //map <word -> map<docId, TF> >
    map<string, map<int, double>> word_index_;
    set<string> stop_words_;
    
    //Inverse Document Frequency (per word per all docs)
    double GetIdf(const string& word) const {
        if(word_index_.count(word) == 0) {
            return 0;
        }//log(totalDocs/number of docs containing the word)
        return log(1.0 * GetTotalDocCount()/GetWordInDocsCount(word));
    }
    
    //return total number of stored documents
    int GetTotalDocCount() const {
        return no_of_docs;
    }
    
    //return number of docs containing word
    int GetWordInDocsCount(const string& word) const {
        if(word_index_.count(word) == 0) {
            return 0;
        }
        return static_cast<int>(word_index_.at(word).size());
    }
    
    //return list of docs containing at least one minus word
    set<int> GetMinusDocs(const vector<QueryWord>& query_words) const {
        set<int> minus_list;
        for(const auto& qword : query_words) {
            if(qword.is_minus && word_index_.count(qword.str) != 0) {
                for(const auto [docId, count] : word_index_.at(qword.str))
                minus_list.insert(docId);
            }
        }
        return minus_list;
    }
    
    //upd for minus words
    bool IsStopWord(const string& word) const {
        //(minus word AND a stop word) OR is a stop word
        return (word[0] == '-' && stop_words_.count(word.substr(1)) > 0)
        || stop_words_.count(word) > 0;
    }
    
    //ignores stop words
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
    
    vector<QueryWord> ParseQuery(const string& text) const {
        vector<QueryWord> query_words;
        for (const auto& word : SplitIntoWordsNoStop(text)) {
            QueryWord qword(word);
            //found a minus-word
            if(qword.str[0] == '-') {
                qword.str.erase(0,1);
                qword.is_minus = true;
            }
            query_words.push_back(word);
        }
        return query_words;
    }
  
    vector<Document> FindAllDocumentsNoMinus(const vector<QueryWord>& query_words) const {
        set<int> minus_list = GetMinusDocs(query_words);
        map<int, double> id_relevance;
        //process each query word in turn
        for(const auto& qword : query_words) {
            if(word_index_.count(qword.str) != 0) {
                double inverse_doc_freq = GetIdf(qword.str); //once per word
                for(const auto& [docId, term_freq] : word_index_.at(qword.str)) {
                    if(minus_list.count(docId) == 0) {//skip docs with -words
                        id_relevance[docId] += inverse_doc_freq*term_freq;
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
