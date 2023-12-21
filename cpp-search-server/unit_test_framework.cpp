#include "unit_test_framework.h"

using std::string;
using std::vector;

using std::cerr;
using std::endl;
using std::operator""s;

void LogImpl(const string& str, const string& func_name, const string& file_name, int line_number) {
    cerr << file_name << "("s << line_number << "): "s;
    cerr << func_name << ": "s << str << endl;
}

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

template <typename TestFunc>
void RunTestImpl(const TestFunc& func, const string& test_name) {
    func();
    cerr << test_name << " OK"s << endl;
}

// -------- Начало модульных тестов поисковой системы ----------

void TestDocumentAdd() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server("a"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    {
        SearchServer server("a"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    {
        SearchServer server("a"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("dog"s).empty(),
                    "There should be no documents found with the word [dog]"s);
    }
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server("a"s);
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

void TestMinusWords() {
    const int doc_id1 = 41;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = {1, 2, 3};
    
    const int doc_id2 = 42;
    const string content2 = "dog in the park"s;
    const vector<int> ratings2 = {1, 2, 3};
    {
        SearchServer server("a"s);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("in -park"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id1);
    }
}

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
        SearchServer server("a"s);
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
        SearchServer server("a"s);
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
        SearchServer server("a"s);
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

void TestDocumentSortByRelevance() {
    const int doc_id1 = 41;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = {1, 2, 3};
    
    const int doc_id2 = 42;
    const string content2 = "dog in the park"s;
    const vector<int> ratings2 = {1, 2, 3};
    
    const string query = "in the city"s;
    {//return in correct order
        SearchServer server("a"s);
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

void TestRatingCompute() {
    const int doc_id1 = 41;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = {1, 2, 3}; // 6/3 = 2;
    
    const int doc_id2 = 42;
    const string content2 = "dog in the park"s;
    const vector<int> ratings2 = {1, 3, 5}; // 9/3 = 3;
    
    const string query = "in the city"s;
    {//return of correct retings
        SearchServer server("a"s);
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

void TestRelevanceCompute() {
    const int doc_id = 41;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3}; // 6/3 = 2;
    
    const int doc_id2 = 42;
    const string content2 = "dog in the park"s;
    const vector<int> ratings2 = {1, 3, 5}; // 9/3 = 3;
    
    
    {
        SearchServer server("a"s);
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
    const vector<string> plus_words_required{"cat"s};
    
    {
        SearchServer server("a"s);
        try{
            server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
            server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
        } catch (std::exception& ex){
            ASSERT_EQUAL(ex.what(), INVALID_ID_MSG);
        }
        try{
            server.FindTopDocuments(bad_query1);
        } catch (std::exception& ex){
            ASSERT_EQUAL(ex.what(), QUERY_WRONG_FORMAT_MSG);
        }
        try{
            server.FindTopDocuments(bad_query2);
        } catch (std::exception& ex){
            ASSERT_EQUAL(ex.what(), QUERY_WRONG_FORMAT_MSG);
        }
        try{
            server.FindTopDocuments(good_query1);
            server.FindTopDocuments(good_query2);
        } catch (std::exception& ex){
            ASSERT_HINT(false, "Error thrown by FindTopDocuments for a valid query"s);
        }
    }
}

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
