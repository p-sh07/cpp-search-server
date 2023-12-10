//
//  04.Базовые понятия С++ и STL
//  Тема 15.Обработка ошибок. Исключения
//  Задача 06.Обработка ошибок в поисковой системе
//
//  Created by Pavel Sh on 10.12.2023.
//
#include "unit_test_framework.hpp"
#include "search_server.hpp"

// P.S.: Код сервера + юнит тестов непоменрно разросся, поэтому был перенесен в отдельные файлы
// [class SearchServer -> search_server.hpp & search_server.cpp]

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    std::cerr << "Search server testing finished"s << endl;
    //OptionalUseExample();
    
}
