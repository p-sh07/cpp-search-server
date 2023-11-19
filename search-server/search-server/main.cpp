// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
// 271
// Закомитьте изменения и отправьте их в свой репозиторий.

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
    void SetDocNumber(int doc_num) {
        doc_word_data_ = vector<int>(doc_num, 0);
    }
    
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
    vector<int> doc_word_data_; // doc count = doc_word_data_.size();
    //map <word -> map<docId, count> >
    map<string, map<int, int>> word_index_;
    set<string> stop_words_;
    
    //inverse document frequency (for word in all docs)
    double get_idf(const string& word) const {
        if(word_index_.count(word) == 0)
            return 0;
        //log(totalDocs/number of docs containing the word)
        return log(1.0 * get_total_doc_count()/get_word_in_docs_count(word));
    }
    
    //term frequency (for query words in each document)
    double get_tf(int doc_id, int count) const {
        //count total words in doc / divide by count in current document:
        if(doc_word_data_.size() <= doc_id) {
            //the doc_id is outside of range stored in SearchServer
            throw(std::out_of_range(""));
        }
        //divide the times count in the doc by total words in the doc
        return 1.0*count/double(doc_word_data_[doc_id]);
    }
    
    int get_total_doc_count() const {
        return int(doc_word_data_.size());
    }
    
    int get_word_in_docs_count(const string& word) const {
        if(word_index_.count(word) == 0) {
            return 0;
        }
        return int(word_index_.at(word).size());
    }
    
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
    
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        //will ignore stop words even if with a '-' suffix
        for (const string& word :  SplitIntoWords(text)) {
            //is a minus word, check that it is not a stop word
            if(word[0] == '-' && !IsStopWord(word.substr(1))) {
                words.push_back(word);
            } //no minus and not a stop word
            else if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    set<string> ParseQuery(const string& text, set<string>& minusWords) const {
        set<string> query_words;
        
        for (const string& word : SplitIntoWordsNoStop(text)) {
            //found a minus-word
            if(word[0] == '-') {
                //cut off the -
                minusWords.insert(word.substr(1));
            }
            else {
                query_words.insert(word);
            }
        }
        return query_words;
    }
  
    vector<Document> FindAllDocumentsNoMinus(const set<string>& query_words, const set<string>& minus_words) const
    {
        //exclude minus words first?
        set<int> minus_list;
        //remove docs with minus words
        for(const auto& word : minus_words) {
            if(word_index_.count(word) != 0) {
                for(const auto [docId, count] : word_index_.at(word))
                minus_list.insert(docId);
            }
        }
        //find all relevant documents
        map<int, double> id_relevance;
        for(const auto& word : query_words) {
            if(word_index_.count(word) != 0) {
                //the word is contained in at least 1 document, so get idf, once per word
                double idf = get_idf(word);
                //for every word match increase relevance
                for(const auto& [docId, count] : word_index_.at(word)) {
                    //no minus words in doc
                    if(minus_list.count(docId) == 0) {
                        //find the tf in this particular document, once per word per doc
                        double tf = get_tf(docId, count);
                        //relevance is sum of TF * IDF for each word in query
                        id_relevance[docId] += idf*tf;
                    }
                }
            }
        }
        //transform into a vector
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
