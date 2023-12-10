//
//  04.Базовые понятия С++ и STL
//  Тема 15.Обработка ошибок. Исключения
//  Задача 03.Использование std::optional (+фреймворк юнит тестов)
//
//  Created by Pavel Sh on 10.12.2023.
//

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <optional>
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
// ==================== unit tests ==========================
/*
 Подставьте сюда вашу реализацию макросов
 ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
 */
// Функция LogImpl выполняет основную работу
void LogImpl(const string& str, const string& func_name, const string& file_name, int line_number) {
    cerr << file_name << "("s << line_number << "): "s;
    cerr << func_name << ": "s << str << endl;
}

#define LOG(expr) LogImpl(#expr, __FUNCTION__, __FILE__, __LINE__)

void AssertImpl(const bool t, const string& t_str, const string& file,
                const string& func, unsigned line, const string& hint) {
    if (t != true) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << t_str << ") failed: "s;
        cerr << t << " != True."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        //        abort();
    }
}

#define ASSERT(a) AssertImpl((a), #a,  __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(a, hint) AssertImpl((a), #a, __FILE__, __FUNCTION__, __LINE__, (hint))


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        //        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunc>
void RunTestImpl(const TestFunc& func, const string& test_name) {
    func();
    cerr << test_name << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)
// -------- Начало модульных тестов поисковой системы ----------

/*
 Разместите код остальных тестов здесь
 */
//Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestDocumentAdd() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        ASSERT(server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings));
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs->size(), 1u);
        const Document& doc0 = found_docs.value()[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    {
        SearchServer server;
        ASSERT(server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings));
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT_EQUAL(found_docs->size(), 1u);
        const Document& doc0 = found_docs.value()[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    {
        SearchServer server;
        ASSERT(server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings));
        ASSERT_HINT(server.FindTopDocuments("dog"s)->empty(),
                    "There should be no documents found with the word [dog]"s);
    }
}
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        ASSERT(server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings));
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs->size(), 1u);
        const Document& doc0 = found_docs.value()[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    
    {
        SearchServer server;
        ASSERT(server.SetStopWords("in the"s));
        ASSERT(server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings));
        ASSERT_HINT(server.FindTopDocuments("in"s)->empty(),
                    "Stop words must be excluded from documents"s);
    }
}
//Поддержка минус-слов. Документы, содержащие минус-слова из поискового запроса, не должны включаться в результаты поиска.
void TestMinusWords() {
    const int doc_id1 = 41;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = {1, 2, 3};
    
    const int doc_id2 = 42;
    const string content2 = "dog in the park"s;
    const vector<int> ratings2 = {1, 2, 3};
    {
        SearchServer server;
        ASSERT(server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1));
        ASSERT(server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2));
        const auto found_docs = server.FindTopDocuments("in -park"s);
        ASSERT_EQUAL(found_docs->size(), 1u);
        const Document& doc0 = found_docs.value()[0];
        ASSERT_EQUAL(doc0.id, doc_id1);
    }
}
//Соответствие документов поисковому запросу. При этом должны быть возвращены все слова из поискового запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestMatchDocumentReturn() {
    const int doc_id1 = 41;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = {1, 2, 3};
    
    const int doc_id2 = 42;
    const string content2 = "dog in the park"s;
    const vector<int> ratings2 = {1, 2, 3};
    
    const string query = "cat"s;
    const vector<string> plus_words_required{"cat"};
    {//return of correct words
        SearchServer server;
        ASSERT(server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1));
        ASSERT(server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2));
        const auto found_docs = server.FindTopDocuments(query);
        ASSERT_EQUAL(found_docs->size(), 1u);
        const Document& doc0 = found_docs.value()[0];
        ASSERT_EQUAL(doc0.id, doc_id1);
        const vector<string> plus_words = std::get<0>(server.MatchDocument(query, doc_id1).value());
        ASSERT_EQUAL(plus_words.size(), plus_words_required.size());
        ASSERT_EQUAL_HINT(plus_words[0], plus_words_required[0], "Should return all matching plus words from query"s);
    }
    const string query2 = "dog -park"s;
    {//return empty when minus word present
        SearchServer server;
        ASSERT(server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1));
        ASSERT(server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2));
        ASSERT(server.FindTopDocuments(query2)->empty());
        const vector<string> plus_words = std::get<0>(server.MatchDocument(query2, doc_id2).value());
        ASSERT_HINT(plus_words.empty(), "Should return empty vector when minus words found in document"s);
    }
    const string query3 = "dog"s;
    const vector<string> plus_words_required3{"dog"};
    const DocumentStatus required_status = DocumentStatus::BANNED;
    {//return of correct words with doc status other than ACTUAL & returns correct status
        SearchServer server;
        ASSERT(server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1));
        ASSERT(server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2));
        const auto found_docs = server.FindTopDocuments(query3, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs->size(), 1u);
        const Document& doc0 = found_docs.value()[0];
        ASSERT_EQUAL(doc0.id, doc_id2);
        const auto match_return = server.MatchDocument(query3, doc_id2);
        const vector<string>& plus_words = std::get<0>(match_return.value());
        const DocumentStatus& status = std::get<DocumentStatus>(match_return.value());
        ASSERT_EQUAL(plus_words.size(), plus_words_required3.size());
        ASSERT_EQUAL_HINT(plus_words[0], plus_words_required3[0], "Should return all matching plus words from query for any document status"s);
        ASSERT_HINT(status == required_status, "MatchDocument replaces document status!");
    }
}
//Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestDocumentSortByRelevance() {
    const int doc_id1 = 41;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = {1, 2, 3};
    
    const int doc_id2 = 42;
    const string content2 = "dog in the park"s;
    const vector<int> ratings2 = {1, 2, 3};
    
    const string query = "in the city"s;
    {//return in correct order
        SearchServer server;
        ASSERT(server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1));
        ASSERT(server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2));
        const auto found_docs = server.FindTopDocuments(query);
        ASSERT_EQUAL(found_docs->size(), 2u);
        const Document& doc0 = found_docs.value()[0];
        const Document& doc1 = found_docs.value()[1];
        ASSERT_EQUAL(doc0.id, doc_id1);
        ASSERT_EQUAL(doc1.id, doc_id2);
        ASSERT_HINT(doc0.relevance > doc1.relevance, "Return documents should be sorted by decreasing relevance"s);
    }
}
//Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestRatingCompute() {
    const int doc_id1 = 41;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = {1, 2, 3}; // 6/3 = 2;
    
    const int doc_id2 = 42;
    const string content2 = "dog in the park"s;
    const vector<int> ratings2 = {1, 3, 5}; // 9/3 = 3;
    
    const string query = "in the city"s;
    {//return of correct retings
        SearchServer server;
        ASSERT(server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1));
        ASSERT(server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2));
        const auto found_docs = server.FindTopDocuments(query);
        ASSERT_EQUAL(found_docs->size(), 2u);
        const Document& doc0 = found_docs.value()[0];
        const Document& doc1 = found_docs.value()[1];
        
        ASSERT_EQUAL(doc0.rating, 2);
        ASSERT_EQUAL(doc1.rating, 3);
    }
}
//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestUserDefinedPredicateFilter(){
    {
        {//return by status
            SearchServer search_server;
            search_server.SetStopWords("и в на"s);
            (void) search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {1});
            (void) search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {2});
            (void) search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {3});
            (void) search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {4});
            
            const auto found_docs_status_banned =  search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
            
            ASSERT_EQUAL(found_docs_status_banned->size(), 1u);
            const Document& doc0 = found_docs_status_banned.value()[0];
            ASSERT_EQUAL_HINT(doc0.id, 3, "Needs to return user-defined filered docs (Status == Banned)");
            
        }
        {//return above sertain rating
            SearchServer search_server;
            search_server.SetStopWords("и в на"s);
            (void) search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {1});
            (void) search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {2});
            (void) search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {3});
            (void) search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {4});
            
            const auto found_docs_rating_above2 =  search_server.FindTopDocuments("пушистый ухоженный кот и пёс"s, [](int document_id, DocumentStatus status, int rating) { return rating > 2; });
            ASSERT_EQUAL(found_docs_rating_above2->size(), 2u);
            const Document& doc0 = found_docs_rating_above2.value()[0];
            const Document& doc1 = found_docs_rating_above2.value()[1];
            
            ASSERT_EQUAL(doc0.id, 2);
            ASSERT_EQUAL(doc1.id, 3);
            
            ASSERT_EQUAL_HINT(doc0.rating, 3, "Needs to return user-defined filered docs (Rating > 2)");
            ASSERT_EQUAL_HINT(doc1.rating, 4, "Needs to return user-defined filered docs (Rating > 2)");
        }
        
    }
}
//Поиск документов, имеющих заданный статус.
void TestStatusFilter() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    (void) search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {1});
    (void) search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {2});
    (void) search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {3});
    (void) search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {4});
    {//return actual by default
        const auto found_docs_actual =  search_server.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT_EQUAL(found_docs_actual->size(), 2u);
        const Document& doc0 = found_docs_actual.value()[0];
        const Document& doc1 = found_docs_actual.value()[1];
        
        ASSERT_EQUAL_HINT(doc0.id, 1, "Failed to return ACTUAL by default");
        ASSERT_EQUAL_HINT(doc1.id, 0, "Failed to return ACTUAL by default");
    }
    {//return IRRELEVANT
        const auto found_docs_irrelevant =  search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs_irrelevant->size(), 1u);
        const Document& doc0 = found_docs_irrelevant.value()[0];
        
        ASSERT_EQUAL_HINT(doc0.id, 2, "Failed to return IRRELEVANT");
    }
}
//Корректное вычисление релевантности найденных документов.
void TestRelevanceCompute() {
    const int doc_id = 41;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3}; // 6/3 = 2;
    
    const int doc_id2 = 42;
    const string content2 = "dog in the park"s;
    const vector<int> ratings2 = {1, 3, 5}; // 9/3 = 3;
    
    
    {
        SearchServer server;
        (void) server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        (void) server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs->size(), 1u);
        const Document& doc0 = found_docs.value()[0];
        ASSERT_EQUAL(doc0.id, doc_id);
        
        const double relevance = (log(2.0/1)* 0.25);
        ASSERT_EQUAL(doc0.relevance, relevance);
    }
    
}
void TestErrorReporting() {
    const int doc_id1 = 41;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = {1, 2, 3};
    
    const int doc_id2 = 42;
    const string content2 = "dog in the park"s;
    const vector<int> ratings2 = {1, 2, 3};
    
    const string good_query = "cat"s;
    
    const string bad_query1 = "cat-"s;
    const string bad_query2 = "cat -"s;
    const string bad_query3 = "cat --the"s;
    const vector<string> plus_words_required{"cat"};
    {
        SearchServer server;
        ASSERT(server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1));
        ASSERT(!server.AddDocument(doc_id1, content2, DocumentStatus::BANNED, ratings2));
        ASSERT(server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2));
        
        ASSERT(!server.MatchDocument(bad_query1, doc_id1));
        ASSERT(!server.MatchDocument(bad_query2, doc_id1));
        ASSERT(!server.MatchDocument(bad_query3, doc_id1));

        const auto match_return = server.MatchDocument(good_query, doc_id1);
        ASSERT(match_return.has_value());
        auto& words = std::get<0>(match_return.value());
        ASSERT_EQUAL(words[0], "cat"s);
    }
}
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestDocumentAdd);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchDocumentReturn);
    RUN_TEST(TestDocumentSortByRelevance);
    RUN_TEST(TestRatingCompute);
    RUN_TEST(TestUserDefinedPredicateFilter);
    RUN_TEST(TestStatusFilter);
    RUN_TEST(TestRelevanceCompute);
    RUN_TEST(TestErrorReporting);
}

// --------- Окончание модульных тестов поисковой системы -----------

// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
    << "document_id = "s << document.id << ", "s
    << "relevance = "s << document.relevance << ", "s
    << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cerr << "Search server testing finished"s << endl;
    
    SearchServer search_server("и в на"s);
    // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
    // о неиспользуемом результате его вызова
    (void) search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    if (!search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2})) {
        cout << "Документ не был добавлен, так как его id совпадает с уже имеющимся"s << endl;
    }
    if (!search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2})) {
        cout << "Документ не был добавлен, так как его id отрицательный"s << endl;
    }
    if (!search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, {1, 3, 2})) {
        cout << "Документ не был добавлен, так как содержит спецсимволы"s << endl;
    }
    if (const auto documents = search_server.FindTopDocuments("--пушистый"s)) {
        for (const Document& document : *documents) {
            PrintDocument(document);
        }
    } else {
        cout << "Ошибка в поисковом запросе"s << endl;
    }
}
