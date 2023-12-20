#pragma once

#include "document.h"
#include "search_server.h"

using std::operator""s;

void LogImpl(const std::string& str, const std::string& func_name, const std::string& file_name, int line_number);

#define LOG(expr) LogImpl(#expr, __FUNCTION__, __FILE__, __LINE__)

void AssertImpl(const bool t, const std::string& t_str, const std::string& file, const std::string& func, unsigned line, const std::string& hint);

#define ASSERT(a) AssertImpl((a), #a,  __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(a, hint) AssertImpl((a), #a, __FILE__, __FUNCTION__, __LINE__, (hint))


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file, const std::string& func, unsigned line, const std::string& hint);

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunc>
void RunTestImpl(const TestFunc& func, const std::string& test_name);

#define RUN_TEST(func) RunTestImpl(func, #func)

// -------- Начало модульных тестов поисковой системы ----------
//Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestDocumentAdd();
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent();
//Поддержка минус-слов. Документы, содержащие минус-слова из поискового запроса, не должны включаться в результаты поиска.
void TestMinusWords();
//Соответствие документов поисковому запросу. При этом должны быть возвращены все слова из поискового запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestMatchDocumentReturn();
//Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestDocumentSortByRelevance();
//Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestRatingCompute();
//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestUserDefinedPredicateFilter();
//Поиск документов, имеющих заданный статус.
void TestStatusFilter();
//Корректное вычисление релевантности найденных документов.
void TestRelevanceCompute();
void TestErrorReporting();
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();

// --------- Окончание модульных тестов поисковой системы -----------
