//
//  unit_test_framework.hpp
//  cpp-search-server
//  Separate hpp file for all the unit-testing code
//
//  Created by Pavel Sh on 10.12.2023.
//

#ifndef unit_test_framework_h
#define unit_test_framework_h

#include "search_server.hpp"

using std::string;
using std::vector;

using std::cerr;
using std::endl;
using std::operator""s;

// ==================== unit tests ==========================
void LogImpl(const string& str, const string& func_name, const string& file_name, int line_number) {
    cerr << file_name << "("s << line_number << "): "s;
    cerr << func_name << ": "s << str << endl;
}

#define LOG(expr) LogImpl(#expr, __FUNCTION__, __FILE__, __LINE__)

void AssertImpl(const bool t, const string& t_str, const string& file,
                const string& func, unsigned line, const string& hint) {
    if (t != true) {
        cerr << std::boolalpha;
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
        cerr << std::boolalpha;
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
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("dog"s).empty(),
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
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
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
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("in -park"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
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
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
        const auto found_docs = server.FindTopDocuments(query);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id1);
        const vector<string> plus_words = std::get<0>(server.MatchDocument(query, doc_id1));
        ASSERT_EQUAL(plus_words.size(), plus_words_required.size());
        ASSERT_EQUAL_HINT(plus_words[0], plus_words_required[0], "Should return all matching plus words from query"s);
    }
    const string query2 = "dog -park"s;
    {//return empty when minus word present
        SearchServer server;
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
        ASSERT(server.FindTopDocuments(query2).empty());
        const vector<string> plus_words = std::get<0>(server.MatchDocument(query2, doc_id2));
        ASSERT_HINT(plus_words.empty(), "Should return empty vector when minus words found in document"s);
    }
    const string query3 = "dog"s;
    const vector<string> plus_words_required3{"dog"};
    const DocumentStatus required_status = DocumentStatus::BANNED;
    {//return of correct words with doc status other than ACTUAL & returns correct status
        SearchServer server;
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
        const auto found_docs = server.FindTopDocuments(query3, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id2);
        const auto match_return = server.MatchDocument(query3, doc_id2);
        const vector<string>& plus_words = std::get<0>(match_return);
        const DocumentStatus& status = std::get<DocumentStatus>(match_return);
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
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments(query);
        ASSERT_EQUAL(found_docs.size(), 2u);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
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
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments(query);
        ASSERT_EQUAL(found_docs.size(), 2u);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        
        ASSERT_EQUAL(doc0.rating, 2);
        ASSERT_EQUAL(doc1.rating, 3);
    }
}
//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestUserDefinedPredicateFilter(){
    {
        {//return by status
            SearchServer search_server("и в на"s);
            search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {1});
            search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {2});
            search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {3});
            search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {4});
            
            const auto found_docs_status_banned =  search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
            
            ASSERT_EQUAL(found_docs_status_banned.size(), 1u);
            const Document& doc0 = found_docs_status_banned[0];
            ASSERT_EQUAL_HINT(doc0.id, 3, "Needs to return user-defined filered docs (Status == Banned)");
            
        }
        {//return above sertain rating
            SearchServer search_server("и в на"s);
            search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {1});
            search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {2});
            search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {3});
            search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {4});
            
            const auto found_docs_rating_above2 =  search_server.FindTopDocuments("пушистый ухоженный кот и пёс"s, [](int document_id, DocumentStatus status, int rating) { return rating > 2; });
            ASSERT_EQUAL(found_docs_rating_above2.size(), 2u);
            const Document& doc0 = found_docs_rating_above2[0];
            const Document& doc1 = found_docs_rating_above2[1];
            
            ASSERT_EQUAL(doc0.id, 2);
            ASSERT_EQUAL(doc1.id, 3);
            
            ASSERT_EQUAL_HINT(doc0.rating, 3, "Needs to return user-defined filered docs (Rating > 2)");
            ASSERT_EQUAL_HINT(doc1.rating, 4, "Needs to return user-defined filered docs (Rating > 2)");
        }
        
    }
}
//Поиск документов, имеющих заданный статус.
void TestStatusFilter() {
    SearchServer search_server("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {1});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {2});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {3});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {4});
    {//return actual by default
        const auto found_docs_actual =  search_server.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT_EQUAL(found_docs_actual.size(), 2u);
        const Document& doc0 = found_docs_actual[0];
        const Document& doc1 = found_docs_actual[1];
        
        ASSERT_EQUAL_HINT(doc0.id, 1, "Failed to return ACTUAL by default");
        ASSERT_EQUAL_HINT(doc1.id, 0, "Failed to return ACTUAL by default");
    }
    {//return IRRELEVANT
        const auto found_docs_irrelevant =  search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs_irrelevant.size(), 1u);
        const Document& doc0 = found_docs_irrelevant[0];
        
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
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
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
    
    const string good_query1 = "cat"s;
    const string good_query2 = "in the -park"s;
    const string good_query3 = "cat-neko"s;
    
    const string bad_query1 = "cat -"s;
    const string bad_query2 = "cat --the"s;
    const vector<string> plus_words_required{"cat"};
    {
        SearchServer server;
        try{
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
            server.AddDocument(doc_id1, content2, DocumentStatus::BANNED, ratings2);
            server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
            
            server.MatchDocument(bad_query1, doc_id1);
            server.MatchDocument(bad_query2, doc_id1);
            
            server.MatchDocument(good_query1, doc_id1);
            server.MatchDocument(good_query2, doc_id1);
            server.MatchDocument(good_query3, doc_id1);
            
        } catch (std::exception& ex){
            cerr << "Error in MatchDocument: " << ex.what() << endl;
        }

        const auto match_return = server.MatchDocument(good_query1, doc_id1);
        auto& words = std::get<0>(match_return);
        ASSERT_EQUAL(words[0], "cat"s);
    }
    {
        SearchServer server;
        try{
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
            server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
        } catch (std::exception& ex){
            cerr << "Error in FindTopDocuments: " << ex.what() << endl;
        }
        try{
            server.FindTopDocuments(bad_query1);
        } catch (std::exception& ex){
            cerr << "Error in FindTopDocuments: " << ex.what() << endl;
        }
        try{
            server.FindTopDocuments(bad_query2);
        } catch (std::exception& ex){
            cerr << "Error in FindTopDocuments: " << ex.what() << endl;
        }
        try{
            server.FindTopDocuments(good_query1);
            server.FindTopDocuments(good_query2);
        } catch (std::exception& ex){
            cerr << "Error in FindTopDocuments: " << ex.what() << endl;
        }

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
    std::cout << "{ "s
    << "document_id = "s << document.id << ", "s
    << "relevance = "s << document.relevance << ", "s
    << "rating = "s << document.rating << " }"s << endl;
}
/*
void OptionalUseExample() {
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
*/


#endif /* unit_test_framework_h */
