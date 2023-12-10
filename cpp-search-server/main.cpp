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

#include "unit_test_framework.hpp"
#include "search_server.hpp"

// P.S.: Код сервера + юнит тестов непоменрно разросся, поэтому был перенесен в отдельные файлы
// [class SearchServer -> search_server.hpp]
// (да, пока возможно этого не стоило делать, и сделано на данном этапе мягко говоря не идеально,
// но так немного удобнее работать =) Надеюсь, на ревью это не повлияет!

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    std::cerr << "Search server testing finished"s << endl;
    OptionalUseExample();
    
}
