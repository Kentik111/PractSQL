#include "../headers/parser.hpp"

Pars_SQL::Pars_SQL(CONFIG& configuration) : config(configuration) {}

// Основная функция для обработки SQL-запросов
void Pars_SQL::parse(const string& query) {
  istringstream stream(query);
  string command;
  stream >> command;

  if (command == "INSERT") {
    handleInsert(stream);
  } else if (command == "SELECT") {
    handleSelect(stream);
  } else if (command == "DELETE") {
    handleDelete(stream);
  } else {
    cerr << "Неизвестная команда: " + command;
    return;
  }
}

void Pars_SQL::handleInsert(istringstream& stream) {
  string into;
  stream >> into;

  if (into != "INTO") {
    throw runtime_error("Ожидалось 'INTO', но получено: " + into);
  }

  string tableName;
  stream >> tableName;

  string values;
  stream >> values;  // Ожидаем значения

  if (values != "VALUES") {
    throw runtime_error("Ожидалось 'VALUES', но получено: " + values);
  }

  char openParen;
  stream >> openParen;  // Ожидаем открывающуюся скобку

  // Проверяем наличие скобки
  if (openParen != '(') {
    throw runtime_error("Ошибка " +
                        string(1, openParen));
  }

  string valuesList;
  if (!getline(stream, valuesList, ')')) {
    throw runtime_error("Ошибка ");
  }

  // Разбор списка значений, соблюдая структуру кавычек
  LinkedList<string> parsedValues = parseValues(valuesList);

  // Вставляем значения в таблицу
  config.insertIntoTable(tableName, parsedValues);
}

// Обработка команды SELECT
void Pars_SQL::handleSelect(istringstream& stream) {
  string fullQuery;

  // Считываем всё содержимое потока до слова "FROM"
  getline(stream, fullQuery);

  // Ищем "FROM"
  size_t fromPos = fullQuery.find("FROM");

  if (fromPos == string::npos) {
    throw runtime_error("Ошибка: не найдено ключевое слово FROM");
  }

  // Извлекаем список колонок до "FROM"
  string columnsPart = fullQuery.substr(0, fromPos);

  // Удаляем лишние пробелы в списке колонок
  columnsPart.erase(0, columnsPart.find_first_not_of(" \t\n\r\f\v"));
  columnsPart.erase(columnsPart.find_last_not_of(" \t\n\r\f\v") + 1);

  // Разбираем колонки по запятой
  istringstream columnStream(columnsPart);
  LinkedList<string> columns;
  string column;

  while (getline(columnStream, column, ',')) {
    // Удаляем лишние пробелы
    column.erase(0, column.find_first_not_of(" \t\n\r\f\v"));  // Пробелы слева
    column.erase(column.find_last_not_of(" \t\n\r\f\v") + 1);  // Пробелы справа

    if (!column.empty()) {
      columns.push_back(column);
    }
  }

  // Теперь обрабатываем таблицы и условия
  size_t wherePos = fullQuery.find("WHERE", fromPos);

  // Извлекаем часть с таблицами после "FROM"
  string tablesPart;
  if (wherePos != string::npos) {
    tablesPart = fullQuery.substr(fromPos + 4, wherePos - (fromPos + 4));
  } else {
    tablesPart = fullQuery.substr(fromPos + 4);  // Всё, что после "FROM"
  }

  // Удаляем лишние пробелы в списке таблиц
  tablesPart.erase(0, tablesPart.find_first_not_of(" \t\n\r\f\v"));
  tablesPart.erase(tablesPart.find_last_not_of(" \t\n\r\f\v") + 1);

  // Разбираем таблицы, игнорируя запятые
  istringstream tableStream(tablesPart);
  LinkedList<string> tables;
  string tableName;

  while (tableStream >> tableName) {
    // Удаляем возможные запятые из имен таблиц
    tableName.erase(remove(tableName.begin(), tableName.end(), ','),
                    tableName.end());
    tables.push_back(tableName);
  }

  // Проверяем, были ли получены таблицы
  if (tables.getSize() == 0) {
    throw runtime_error("Ошибка: не найдены таблицы для запроса SELECT");
  }

  // Проверка, что каждая колонка ссылается на одну из таблиц
  for (int i = 0; i < columns.getSize(); ++i) {
    string col = columns[i];
    size_t dotPos = col.find('.');

    if (dotPos == string::npos) {
      throw runtime_error("Ошибка: колонка " + col +
                          " не содержит имя таблицы");
    }

    string tableInColumn = col.substr(0, dotPos);  // Имя таблицы
    string columnName = col.substr(dotPos + 1);    // Имя колонки

    bool tableFound = false;
    for (int j = 0; j < tables.getSize(); ++j) {
      if (tables[j] == tableInColumn) {
        tableFound = true;
        break;
      }
    }

    if (!tableFound) {
      throw runtime_error("Ошибка: таблица " + tableInColumn + " из колонки " +
                          col + " не указана в списке таблиц");
    }
  }

  if (wherePos != string::npos) {
    // Есть секция WHERE — обрабатываем условия
    string whereClause = fullQuery.substr(wherePos + 5);
    whereClause.erase(0, whereClause.find_first_not_of(" \t\n\r\f\v"));
    whereClause.erase(whereClause.find_last_not_of(" \t\n\r\f\v") + 1);

    // Разбор условий WHERE
    LinkedList<LinkedList<string>> parsedConditions = parseConditions(whereClause);

    // Проверка, что каждая колонка в условиях относится к указанным таблицам
    for (int i = 0; i < parsedConditions.getSize(); ++i) {
      for (int j = 0; j < parsedConditions[i].getSize(); ++j) {
        string col = parsedConditions[i][j];  // Первая колонка в условии
        size_t dotPos = col.find('.');

        if (dotPos == string::npos) {
          throw runtime_error("Ошибка: условие " + col +
                              " не содержит имя таблицы");
        }

        string tableInCondition = col.substr(0, dotPos);  // Имя таблицы

        // Проверяем, указана ли эта таблица в секции FROM
        bool tableFound = false;
        for (int k = 0; k < tables.getSize(); ++k) {
          if (tables[k] == tableInCondition) {
            tableFound = true;
            break;
          } else {
            tableFound = false;
          }
        }

        if (!tableFound) {
          throw runtime_error("Ошибка: таблица " + tableInCondition +
                              " из условия " + col +
                              " не указана в списке таблиц");
        }
      }
    }

    // Применение условий WHERE
    config.applyWhereConditions(tables, columns, parsedConditions);
  } else {
    // Нет секции WHERE — просто обработка без условий
    config.applySelect(tables, columns);
  }
}

void Pars_SQL::handleDelete(istringstream& stream) {
  string fromClause, tableNamesPart, whereClause;

  // Считываем "FROM"
  stream >> fromClause;

  // Считываем всё оставшееся после "FROM", включая возможные пробелы
  getline(stream, tableNamesPart);

  // Ищем позицию "WHERE"
  size_t wherePos = tableNamesPart.find("WHERE");

  if (wherePos != string::npos) {
    // Получаем часть до "WHERE" — это имена таблиц
    string tablesPart = tableNamesPart.substr(0, wherePos);

    // Убираем лишние пробелы
    tablesPart.erase(0, tablesPart.find_first_not_of(" \t\n\r\f\v"));
    tablesPart.erase(tablesPart.find_last_not_of(" \t\n\r\f\v") + 1);

    // Создаем поток для обработки таблиц и удаляем запятые
    istringstream tableStream(tablesPart);
    LinkedList<string> tables;
    string tableName;

    // Разделение по запятой
    while (getline(tableStream, tableName, ',')) {
      // Удаляем пробелы у каждого имени таблицы
      tableName.erase(0, tableName.find_first_not_of(" \t\n\r\f\v"));
      tableName.erase(tableName.find_last_not_of(" \t\n\r\f\v") + 1);
      tables.push_back(tableName);
    }

    // Проверяем, что таблицы были получены
    if (tables.getSize() == 0) {
      throw runtime_error("Ошибка: не найдены таблицы для запроса DELETE");
    }

    // Получаем условия после "WHERE"
    whereClause = tableNamesPart.substr(wherePos + 5);
    whereClause.erase(0, whereClause.find_first_not_of(" \t\n\r\f\v"));
    whereClause.erase(whereClause.find_last_not_of(" \t\n\r\f\v") + 1);

    // Разбор условий
    LinkedList<LinkedList<string>> parsedConditions = parseConditions(whereClause);

    // Проверка, что каждая колонка в условиях ссылается на одну из таблиц
    for (int i = 0; i < parsedConditions.getSize(); ++i) {
      LinkedList<string> condition = parsedConditions[i];
      for (int j = 0; j < condition.getSize(); ++j) {
        string col = condition[j];
        size_t dotPos = col.find('.');

        if (dotPos == string::npos) {
          throw runtime_error("Ошибка: колонка " + col +
                              " не содержит имя таблицы");
        }

        string tableInColumn = col.substr(0, dotPos);  // Имя таблицы
        string columnName = col.substr(dotPos + 1);    // Имя колонки

        bool tableFound = false;
        for (int t = 0; t < tables.getSize(); ++t) {
          if (tables[t] == tableInColumn) {
            tableFound = true;
            break;
          }
        }

        if (!tableFound) {
          throw runtime_error("Ошибка: таблица " + tableInColumn +
                              " из условия " + col +
                              " не указана в списке таблиц");
        }
      }
    }

    // Применение условий удаления к каждой таблице
    config.applyDeleteConditions(tables, parsedConditions);
  } else {
    std::cerr << "Ошибка: не найдено ключевое слово WHERE\n";
  }
}

LinkedList<LinkedList<string>> Pars_SQL::parseConditions(string& query) {
  LinkedList<LinkedList<string>> result;

  // Лямбда-функция для удаления одинарных кавычек из значений
  auto removeQuotes = [](string& condition) -> string {
    size_t pos1 = condition.find('\'');
    size_t pos2 = condition.rfind('\'');

    // Проверяем, есть ли кавычки в начале и конце, и удаляем их
    if (pos1 != string::npos && pos2 != string::npos && pos1 != pos2) {
      condition.erase(pos2, 1);  // Удаляем правую кавычку
      condition.erase(pos1, 1);  // Удаляем левую кавычку
    }

    return condition;
  };

  // Лямбда-функция для разбора части строки с условиями через 'AND'
  auto parseAndConditions =
      [&removeQuotes](const string& subquery) -> LinkedList<string> {
    LinkedList<string> conditions;
    size_t start = 0;
    size_t andPos = subquery.find(" AND ");

    while (andPos != string::npos) {
      // Вырезаем условие между двумя 'AND'
      string condition = subquery.substr(start, andPos - start);
      condition = removeQuotes(condition);  // Удаляем кавычки
      conditions.push_back(condition);      // Добавляем условие

      // Обновляем индексы для следующего поиска
      start = andPos + 5;  // Пропускаем " AND "
      andPos = subquery.find(" AND ", start);
    }

    // Добавляем последнюю часть, которая идёт после последнего 'AND'
    if (start < subquery.size()) {
      string condition = subquery.substr(start);
      condition = removeQuotes(condition);  // Удаляем кавычки
      conditions.push_back(condition);
    }

    return conditions;
  };

  size_t orPos = query.find(" OR ");  // Ищем первое вхождение 'OR'

  if (orPos != string::npos) {
    // Обрабатываем часть до 'OR'
    string leftPart = query.substr(0, orPos);
    result.push_back(
        parseAndConditions(leftPart));  // Разбираем и добавляем левую часть

    // Обрабатываем часть после 'OR'
    string rightPart = query.substr(orPos + 4);  // Обрезаем строку после 'OR'
    result.push_back(
        parseAndConditions(rightPart));  // Разбираем и добавляем правую часть
  } else {
    // Если 'OR' не найден, просто разбираем всю строку
    result.push_back(parseAndConditions(query));
  }

  return result;
}

// в insert используется
LinkedList<string> Pars_SQL::parseValues(const string& valuesList) {
  LinkedList<string> values;
  string cleanedValues = valuesList;

  // Убираем все пробелы в начале и конце строки
  cleanedValues.erase(0, cleanedValues.find_first_not_of(" \t\n\r\f\v"));
  cleanedValues.erase(cleanedValues.find_last_not_of(" \t\n\r\f\v") + 1);

  // Убираем круглые скобки
  if (!cleanedValues.empty() && cleanedValues.front() == '(') {
    cleanedValues.erase(0, 1);
  }
  if (!cleanedValues.empty() && cleanedValues.back() == ')') {
    cleanedValues.erase(cleanedValues.size() - 1);
  }

  size_t start = 0;
  size_t end = 0;

  // Проходим по строке и извлекаем значения между кавычками
  while ((start = cleanedValues.find('\'', end)) != string::npos) {
    end = cleanedValues.find('\'', start + 1);

    if (end != string::npos) {
      string value = cleanedValues.substr(start + 1, end - start - 1);
      values.push_back(value);
      end++;
    }
  }

  return values;
}
