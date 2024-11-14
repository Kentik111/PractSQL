#include "../headers/config.hpp"

#include <filesystem>  
#include <fstream>
#include <stdexcept>

#include "../library/json.hpp"

using json = nlohmann::json;
using namespace std;

// Конструктор класса CONFIG
CONFIG::CONFIG(){};

// Метод для чтения конфигурации из JSON файла
void CONFIG::readConfig(string PathSchema) {
  ifstream file(PathSchema);// Открываем файл конфигурации

  // Проверяем, удалось ли открыть файл
  if (!file.is_open()) {
    throw runtime_error("Не удалось открыть файл: " + PathSchema);
  }

  
  json JFile;// Создаем объект JSON для хранения данных из файла
  file >> JFile;// Считываем данные из файла в объект JSON

  // Проверяем наличие необходимых ключей в JSON файле
  if (JFile.contains("name") && JFile.contains("tuples_limit") &&
      JFile.contains("structure")) {
    schemaName = JFile["name"];// Имя схемы
    tuplesLimit = JFile["tuples_limit"];// Лимит кортежей

    // Проходим по каждой таблице в структуре
    for (auto& JTable : JFile["structure"].items()) {
      Table table;// Создаем объект таблицы
      table.tableName = JTable.key();// Имя таблицы
      table.tuplesLimit = this->tuplesLimit;// Лимит кортежей для таблицы

      CSVFILE csv;  // Создаем объект CSV файла
      // Проходим по каждой колонке в таблице
      for (auto& col : JTable.value()) {
        if (col.is_string()) {
          csv.columns.push_back(col.get<string>());// Добавляем колонку в CSV файл
        } else {
          throw runtime_error("Колонка должна быть строкой");
        }
      }
      table.csv.push_back(csv); // Добавляем CSV файл в таблицу
      // Добавляем имя таблицы в качестве первой колонки в CSV файл
      table.csv[0].columns.insert_beginning(table.tableName + "_pk");

      structure.push_back(table);// Добавляем таблицу в структуру
    }
  } else {
    throw runtime_error("Отсутствуют необходимые ключи в JSON файле");
  }

 
  // Создаем путь к схеме
  string schemaPath = "../" + schemaName;
  // Проверяем, существует ли директория схемы
  if (!filesystem::exists(schemaPath)) {
    createDirectoriesAndFiles();  // Создаем директории и файлы
  }

  file.close();  // Закрываем файл
}


// Метод для загрузки данных существующей схемы
void CONFIG::loadExistingSchemaData(LinkedList<string>& tableNames) {
  // Проходим по каждой таблице в структуре
  for (auto& table : structure) {
    // Проверяем, содержится ли имя таблицы в списке tableNames
    if (!tableNames.contains(table.tableName)) {
      continue;  // Пропускаем таблицу, если ее нет в списке
    }

    // Создаем путь к таблице
    table.pathTable = fs::path("..") / schemaName / table.tableName;

    // Подсчитываем количество CSV файлов в таблице
    table.countCSVFiles();
    // Читаем файл блокировки
    table.readLockFile();
    // Читаем файл последовательности первичных ключей
    table.readPKSequenceFile();

    // Добавляем новые CSV файлы, если их количество меньше, чем подсчитано
    
    while (table.csv.getSize() < table.countCSVFile) {
      string nameCSV = to_string(table.csv.getSize() + 1) + ".csv";
      table.csv.push_back(CSVFILE(nameCSV));
    }

    // Устанавливаем имя первого CSV файла
    table.csv[0].csvName = "1.csv";

    // Проходим по каждому CSV файлу в таблице
    for (int i = 1; i <= table.countCSVFile; ++i) {
      string csvFilePath = fs::path(table.pathTable) / (to_string(i) + ".csv");
      ifstream csvFile(csvFilePath);// Открываем CSV файл

      // Проверяем, удалось ли открыть файл
      if (!csvFile.is_open()) {
        throw runtime_error("Не удалось открыть файл таблицы: " + csvFilePath);
      }

      string line;
      bool isFirstLine = true;// Флаг для первой строки (заголовков)
      // Читаем строки из CSV файла
      while (getline(csvFile, line)) {
        if (line.empty()) continue;  // Пропускаем пустые строки  

        LinkedList<string> row = parseCSVLine(line);// Парсим строку CSV

        // Если это первая строка, сохраняем ее как заголовки
        if (isFirstLine) {
          
          table.csv[i - 1].columns = row;
          isFirstLine = false;// Снимаем флаг первой строки  
        } else {
          
          table.csv[i - 1].line.push_back(row);// Добавляем строку в таблицу
        }
      }

      csvFile.close(); // Закрываем CSV файл
    }
  }
}
// Перегруженный метод для загрузки данных существующей схемы
void CONFIG::loadExistingSchemaData(const LinkedList<string>& tableNames) {
  // Проходим по каждой таблице в структуре
  for (auto& table : structure) {
    // Проверяем, содержится ли имя таблицы в списке tableNames
    if (!tableNames.contains(table.tableName)) {
      continue;  // Пропускаем таблицу, если ее нет в списке
    }
    // Создаем путь к таблице
    table.pathTable = fs::path("..") / schemaName / table.tableName;

    // Подсчитываем количество CSV файлов в таблице
    table.countCSVFiles();

    // Читаем информацию из файла блокировки и pk_sequence
    table.readLockFile();
    table.readPKSequenceFile();

    // Добавляем новые CSV файлы, если их количество меньше, чем подсчитано
    while (table.csv.getSize() < table.countCSVFile) {
      string nameCSV = to_string(table.csv.getSize() + 1) + ".csv";
      table.csv.push_back(CSVFILE(nameCSV));
    }

    // Устанавливаем имя первого CSV файла
    table.csv[0].csvName = "1.csv";

    // Проходим по каждому CSV файлу в таблице
    for (int i = 1; i <= table.countCSVFile; ++i) {
      string csvFilePath = fs::path(table.pathTable) / (to_string(i) + ".csv");
      ifstream csvFile(csvFilePath);

      // Проверяем, удалось ли открыть файл
      if (!csvFile.is_open()) {
        throw runtime_error("Не удалось открыть файл таблицы: " + csvFilePath);
      }

      string line;
      bool isFirstLine = true;  // Флаг для первой строки (заголовков)
      // Читаем строки из CSV файла
      while (getline(csvFile, line)) {
        if (line.empty()) continue;  // Пропускаем пустые строки

        LinkedList<string> row = parseCSVLine(line);

        // Если это первая строка, сохраняем ее как заголовки
        if (isFirstLine) {
          table.csv[i - 1].columns = row;
          isFirstLine = false;  // Снимаем флаг первой строки
        } else {
          table.csv[i - 1].line.push_back(row);  // Добавляем строку в таблицу
        }
      }

      csvFile.close();  // Закрываем CSV файл
    }
  }
}

// Перегруженный метод для загрузки данных существующей схемы
void CONFIG::loadExistingSchemaData(string& tableName) {
  // Проходим по каждой таблице в структуре
  for (auto& table : structure) {
    // Проверяем, совпадает ли имя таблицы с переданным именем
    if (tableName != table.tableName) {
      continue;  // Пропускаем таблицу, если имена не совпадают
    }

    // Создаем путь к таблице
    table.pathTable = fs::path("..") / schemaName / table.tableName;

    // Подсчитываем количество CSV файлов в таблице
    table.countCSVFiles();
    // Читаем файл блокировки
    table.readLockFile();
    // Читаем файл последовательности первичных ключей
    table.readPKSequenceFile();

    // Добавляем новые CSV файлы, если их количество меньше, чем подсчитано
    while (table.csv.getSize() <= table.countCSVFile) {
      string nameCSV = to_string(table.csv.getSize() + 1) + ".csv";
      table.csv.push_back(CSVFILE(nameCSV));
    }

    // Устанавливаем имя первого CSV файла
    table.csv[0].csvName = "1.csv";

    // Проходим по каждому CSV файлу в таблице
    for (int i = 1; i <= table.countCSVFile; ++i) {
      string csvFilePath = fs::path(table.pathTable) / (to_string(i) + ".csv");
      ifstream csvFile(csvFilePath);  // Открываем CSV файл

      // Проверяем, удалось ли открыть файл
      if (!csvFile.is_open()) {
        throw runtime_error("Не удалось открыть файл таблицы: " + csvFilePath);
      }

      string line;
      bool isFirstLine = true;  // Флаг для первой строки (заголовков)
      // Читаем строки из CSV файла
      while (getline(csvFile, line)) {
        if (line.empty()) continue;  // Пропускаем пустые строки

        LinkedList<string> row = parseCSVLine(line);  // Парсим строку CSV

        // Если это первая строка, сохраняем ее как заголовки
        if (isFirstLine) {
          table.csv[i - 1].columns = row;
          isFirstLine = false;  // Снимаем флаг первой строки
        } else {
          table.csv[i - 1].line.push_back(row);  // Добавляем строку в таблицу
        }
      }

      csvFile.close();  // Закрываем CSV файл
    }
  }
}

// Метод для выгрузки данных схемы
void CONFIG::unloadSchemaData(LinkedList<string>& tableNames) {
  // Проходим по каждой таблице в структуре
  for (auto& table : structure) {
    // Проверяем, содержится ли имя таблицы в списке tableNames
    if (!tableNames.contains(table.tableName)) {
      continue;  // Пропускаем таблицу, если ее нет в списке
    }

    // Очищаем данные каждого CSV файла в таблице
    for (auto& csv : table.csv) {
      csv.columns.clear();  // Очищаем заголовки
      csv.line.clear();  // Очищаем строки
    }

    table.csv.clear();  // Очищаем список CSV файлов
    table.countCSVFile = 0;  // Сбрасываем количество CSV файлов
    table.pathTable.clear();  // Очищаем путь к таблице
    table.pk_sequence = 0;  // Сбрасываем последовательность первичных ключей
    table.lock = 0;  // Сбрасываем блокировку
  }
}
// Перегруженный метод для выгрузки данных схемы
void CONFIG::unloadSchemaData(const LinkedList<string>& tableNames) {
  // Проходим по каждой таблице в структуре
  for (auto& table : structure) {
    // Проверяем, содержится ли имя таблицы в списке tableNames
    if (!tableNames.contains(table.tableName)) {
      continue;  // Пропускаем таблицу, если ее нет в списке
    }

    // Очищаем данные каждого CSV файла в таблице
    for (auto& csv : table.csv) {
      csv.columns.clear();  // Очищаем заголовки
      csv.line.clear();  // Очищаем строки
    }

    table.csv.clear();  // Очищаем список CSV файлов
    table.countCSVFile = 0;  // Сбрасываем количество CSV файлов
    table.pathTable.clear();  // Очищаем путь к таблице
    table.pk_sequence = 0;  // Сбрасываем последовательность первичных ключей
    table.lock = 0;  // Сбрасываем блокировку
  }
}

// Перегруженный метод для выгрузки данных схемы
void CONFIG::unloadSchemaData(string& tableNames) {
  // Проходим по каждой таблице в структуре
  for (auto& table : structure) {
    // Проверяем, совпадает ли имя таблицы с переданным именем
    if (tableNames != table.tableName) {
      continue;  // Пропускаем таблицу, если имена не совпадают
    }

    // Очищаем данные каждого CSV файла в таблице
    for (auto& csv : table.csv) {
      csv.columns.clear();  // Очищаем заголовки
      csv.line.clear();  // Очищаем строки
    }

    table.csv.clear();  // Очищаем список CSV файлов
    table.countCSVFile = 0;  // Сбрасываем количество CSV файлов
    table.pathTable.clear();  // Очищаем путь к таблице
    table.pk_sequence = 0;  // Сбрасываем последовательность первичных ключей
    table.lock = 0;  // Сбрасываем блокировку
  }
}
// Метод для парсинга строки CSV
LinkedList<string> CONFIG::parseCSVLine(const string& line) {
  LinkedList<string> parsedRow;  // Создаем список для хранения значений
  stringstream ss(line);  // Создаем поток для чтения
  string value;

  // Разделяем строку по запятым
  while (getline(ss, value, ',')) {
    // Удаляем пробелы по краям значения
    value.erase(0, value.find_first_not_of(" \t\n\r\f\v"));
    value.erase(value.find_last_not_of(" \t\n\r\f\v") + 1);
    parsedRow.push_back(value);  // Добавляем значение в список
  }

  return parsedRow;  // Возвращаем список значений
}

// Метод для создания директорий и файлов
void CONFIG::createDirectoriesAndFiles() {
  fs::path practicaDir = fs::path("..") / schemaName;  // Путь к директории схемы

  // Проверяем, существует ли директория схемы
  if (!fs::exists(practicaDir)) {
    fs::create_directory(practicaDir);  // Создаем директорию схемы
  }

  // Проходим по каждой таблице в структуре
  for (size_t i = 0; i < structure.getSize(); ++i) {
    Table& table = structure[i];  // Получаем текущую таблицу

    fs::path tableDir = practicaDir / table.tableName;  // Путь к директории таблицы
    if (!fs::exists(tableDir)) {
      fs::create_directory(tableDir);  // Создаем директорию таблицы
    }

    table.pathTable = tableDir;  // Устанавливаем путь к таблице

    fs::path csvFile = tableDir / "1.csv";  // Путь к первому CSV файлу
    ofstream file(csvFile);  // Открываем файл для записи
    if (file.is_open()) {
      // Записываем заголовки столбцов в файл
      for (size_t j = 0; j < table.csv[0].columns.getSize(); ++j) {
        file << table.csv[0].columns[j];
        if (j < table.csv[0].columns.getSize() - 1) {
          file << ",";
        }
      }
      file << endl;
      file.close();  // Закрываем файл
    } else {
      cerr << "Ошибка при создании файла: " << csvFile << endl;
    }

    fs::path pkSeqenceFile = tableDir / (table.tableName + "_pk_seqence");  // Путь к файлу последовательности первичных ключей
    if (!fs::exists(pkSeqenceFile)) {
      ofstream file(pkSeqenceFile);  // Открываем файл для записи
      if (file.is_open()) {
        file << table.pk_sequence;  // Записываем начальное значение последовательности
        file.close();  // Закрываем файл
      } else {
        cerr << "Ошибка при создании файла: " << table.tableName + "_pk_seqence" << endl;
      }
    }

    fs::path lockFile = tableDir / (table.tableName + "_lock");  // Путь к файлу блокировки
    if (!fs::exists(lockFile)) {
      ofstream file(lockFile);  // Открываем файл для записи
      if (file.is_open()) {
        file << table.lock;  // Записываем начальное значение блокировки
        file.close();  // Закрываем файл
      } else {
        cerr << "Ошибка при создании файла: " << table.tableName + "_lock" << endl;
      }
    }
  }
}

// Метод для вставки данных в таблицу
void CONFIG::insertIntoTable(string TableName, LinkedList<string> arrValues) {
  try {
    loadExistingSchemaData(TableName);  // Загружаем данные существующей схемы
  } catch (const runtime_error& e) {
    createDirectoriesAndFiles();  // Создаем директории и файлы, если схема не существует
    loadExistingSchemaData(TableName);  // Загружаем данные существующей схемы
  }

  Table& currentTable = searchTable(TableName);  // Ищем таблицу по имени

  currentTable.readLockFile();  // Читаем файл блокировки

  // Ожидаем разблокировки таблицы
  while (currentTable.lock == 1) {
    this_thread::sleep_for(chrono::milliseconds(100));
    cout << "Ожидание разблокировки файла";
    currentTable.readLockFile();  // Обновляем информацию о блокировке
  }

  currentTable.lock = 1;  // Блокируем таблицу
  updateLock(currentTable);  // Обновляем файл блокировки

  ++currentTable.pk_sequence;  // Увеличиваем последовательность первичных ключей
  arrValues.insert_beginning(to_string(currentTable.pk_sequence));  // Добавляем первичный ключ в начало списка значений

  // Проверяем, нужно ли создать новый CSV файл
  if (currentTable.csv.back().line.getSize() >= tuplesLimit) {
    if (currentTable.csv.getSize() == 0) {
      CSVFILE newCsv("1.csv", currentTable.csv.back().columns);
      currentTable.csv.push_back(newCsv);
      currentTable.countCSVFile++;  // Увеличиваем количество CSV файлов
    }

    string newCsvName = to_string(currentTable.csv.getSize() + 1) + ".csv";
    CSVFILE newCsv(newCsvName, currentTable.csv.back().columns.copy());
    currentTable.csv.push_back(newCsv);
    currentTable.countCSVFile++;  // Увеличиваем количество CSV файлов
  }

  // Проверяем количество значений и колонок
  if (arrValues.getSize() == currentTable.csv[0].columns.getSize()) {
    currentTable.csv.back().line.push_back(arrValues);  // Добавляем строку в таблицу
  } else if (arrValues.getSize() < currentTable.csv[0].columns.getSize()) {
    // Дополняем строку значениями "null", если значений меньше, чем колонок
    while (arrValues.getSize() != currentTable.csv[0].columns.getSize()) {
      arrValues.push_back("null");
    }
    currentTable.csv.back().line.push_back(arrValues);  // Добавляем строку в таблицу
  } else {
    cerr << "---аргументов больше чем колонок----\n";
  }

  updateCSVFile(currentTable);  // Обновляем CSV файл

  currentTable.lock = 0;  // Разблокируем таблицу
  updateLock(currentTable);  // Обновляем файл блокировки
  updatePkSeqence(currentTable);  // Обновляем файл последовательности первичных ключей

  unloadSchemaData(TableName);  // Выгружаем данные схемы
}

// Метод для выполнения запроса SELECT
void CONFIG::applySelect(const LinkedList<string>& tableNames, const LinkedList<string>& tableColumns) {
  loadExistingSchemaData(tableNames);  // Загружаем данные существующей схемы

  // Проверяем наличие данных
  if (tableNames.getSize() == 0 || tableColumns.getSize() == 0) {
    cerr << "Ошибка: отсутствуют данные" << endl;
    return;
  }

  LinkedList<Table> involvedTables;  // Список таблиц, участвующих в запросе
  for (const string& tableName : tableNames) {
    try {
      Table table = searchTable(tableName);  // Ищем таблицу по имени

      // Проверяем, пуста ли таблица
      if (table.csv.getSize() == 0) {
        cerr << "Ошибка: таблица " << tableName << " пуста." << endl;
        return;
      }
      involvedTables.push_back(table);  // Добавляем таблицу в список
    } catch (const runtime_error& e) {
      cerr << "Ошибка: не удалось найти таблицу " << tableName << ". " << e.what() << endl;
      return;
    }
  }

  LinkedList<string> crossJoinResult;  // Результат перекрестного соединения

  // Проходим по каждой колонке в запросе
  for (const string& columnString : tableColumns) {
    size_t dotPos = columnString.find('.');
    if (dotPos == string::npos) {
      cerr << "Ошибка: неверный формат имени колонки " << columnString << endl;
      return;
    }

    string tableName = columnString.substr(0, dotPos);  // Имя таблицы
    string columnName = columnString.substr(dotPos + 1);  // Имя колонки

    Table table = searchTable(tableName);  // Ищем таблицу по имени
    LinkedList<string> columnValues = table.getColumnValues(columnName);  // Получаем значения колонки

    // Если результат перекрестного соединения пуст, инициализируем его значениями колонки
    if (crossJoinResult.getSize() == 0) {
      crossJoinResult = columnValues;
    } else {
      crossJoinResult = crossJoin(crossJoinResult, columnValues);  // Выполняем перекрестное соединение
    }
  }

  cout << "Результаты запроса SELECT:" << endl;
  for (const string& row : crossJoinResult) {
    cout << row << endl;  // Выводим результаты запроса
  }

  unloadSchemaData(tableNames);  // Выгружаем данные схемы
}

// Метод для выполнения запроса с условиями WHERE
void CONFIG::applyWhereConditions(const LinkedList<string>& tableNames, const LinkedList<string>& tableColumns, const LinkedList<LinkedList<string>>& conditional) {
  loadExistingSchemaData(tableNames);  // Загружаем данные существующей схемы
  if (conditional.getSize() == 0 || conditional[0].getSize() == 0) {
    cerr << "Ошибка: Условие не указано" << endl;
    return;
  }

  LinkedList<LinkedList<string>> rowFiltration;  // Результат фильтрации строк

  // Обрабатываем таблицы с условиями
  processTableWithConditions(tableNames, conditional, [&](const LinkedList<string>& row) {
    rowFiltration.insert_beginning(row);  // Добавляем строку в результат фильтрации
    row.print();  // Выводим строку
    cout << endl;
  });

  unloadSchemaData(tableNames);  // Выгружаем данные схемы
}
// Метод для выполнения запроса DELETE с условиями
void CONFIG::applyDeleteConditions(const LinkedList<string>& tableNames, const LinkedList<LinkedList<string>>& conditional) {
  loadExistingSchemaData(tableNames);  // Загружаем данные существующей схемы
  if (conditional.getSize() == 0 || conditional[0].getSize() == 0) {
    cerr << "Условие отсутствует" << endl;
    return;
  }

  // Обрабатываем таблицы с условиями
  processTableWithConditions(tableNames, conditional, [&](CSVFILE& currentCSV, int indexToDelete) {
    currentCSV.line.erase(indexToDelete);  // Удаляем строку из CSV файла
  });

  unloadSchemaData(tableNames);  // Выгружаем данные схемы
}

// Метод для обработки таблиц с условиями
void CONFIG::processTableWithConditions(const LinkedList<string>& tableNames, const LinkedList<LinkedList<string>>& conditional, function<void(const LinkedList<string>&)> action) {
  LinkedList<Table> involvedTables;  // Список таблиц, участвующих в запросе

  // Загружаем все указанные таблицы
  for (int i = 0; i < tableNames.getSize(); ++i) {
    involvedTables.push_back(searchTable(tableNames[i]));
  }

  if (involvedTables.getSize() == 0) {
    throw runtime_error("Таблицы не найдено");
  }

  // Проходим по каждой таблице
  for (int t = 0; t < involvedTables.getSize(); ++t) {
    Table& currentTable = involvedTables[t];

    if (currentTable.csv.getSize() == 0) {
      throw runtime_error("Таблица " + tableNames[t] + " пуста");
    }

    // Перебираем все CSV-файлы в текущей таблице
    for (int i = 0; i < currentTable.csv.getSize(); ++i) {
      CSVFILE& currentCSV = currentTable.csv[i];

      // Перебираем строки в CSV файла
      for (int j = currentCSV.line.getSize() - 1; j >= 0; --j) {
        LinkedList<string>& row = currentCSV.line[j];

        bool match = false;

        // Перебираем все группы условий (OR)
        for (int k = 0; k < conditional.getSize(); ++k) {
          const LinkedList<string>& conditionGroup = conditional[k];

          // Проверка условий для данной строки с учетом всех таблиц
          if (checkAndConditionsAcrossTables(conditionGroup, row, currentTable.tableName, i, j)) {
            match = true;

            if (match) {
              action(row);  // Выполняем действие для строки
            }
            break;
          }
        }
      }
    }
  }
}

// Метод для обработки таблиц с условиями
void CONFIG::processTableWithConditions(const LinkedList<string>& tableNames, const LinkedList<LinkedList<string>>& conditional, const function<void(CSVFILE&, int)>& action) {
  LinkedList<Table> involvedTables;  // Список таблиц, участвующих в запросе

  // Загружаем все указанные таблицы
  for (int i = 0; i < tableNames.getSize(); ++i) {
    involvedTables.push_back(searchTable(tableNames[i]));
  }

  if (involvedTables.getSize() == 0) {
    throw runtime_error("Ни одной таблицы не найдено");
  }

  // Проходим по каждой таблице
  for (int t = 0; t < involvedTables.getSize(); ++t) {
    Table& currentTable = involvedTables[t];

    currentTable.readLockFile();  // Читаем файл блокировки

    // Ожидание разблокировки таблицы
    while (currentTable.lock == 1) {
      this_thread::sleep_for(chrono::milliseconds(100));
      cout << "жду разблокировки файла";
      currentTable.readLockFile();  // Обновляем информацию о блокировке
    }

    // Блокируем таблицу для работы
    currentTable.lock = 1;
    updateLock(currentTable);  // Обновляем файл блокировки

    if (currentTable.csv.getSize() == 0) {
      throw runtime_error("Таблица " + tableNames[t] + " пуста");
    }

    // Перебираем все CSV-файлы в текущей таблице
    for (int i = 0; i < currentTable.csv.getSize(); ++i) {
      CSVFILE& currentCSV = currentTable.csv[i];

      // Перебираем строки в CSV файла
      for (int j = currentCSV.line.getSize() - 1; j >= 0; --j) {
        LinkedList<string>& row = currentCSV.line[j];

        bool match = false;

        // Перебираем все группы условий (OR)
        for (int k = 0; k < conditional.getSize(); ++k) {
          const LinkedList<string>& conditionGroup = conditional[k];

          // Проверка условий для данной строки с учетом всех таблиц
          if (checkAndConditionsAcrossTables(conditionGroup, row, currentTable.tableName, i, j)) {
            match = true;

            if (match) {
              action(currentCSV, j);  // Выполняем действие для строки
            }
            break;
          }
        }
      }
    }

    currentTable.lock = 0;  // Разблокируем таблицу
    updateLock(currentTable);  // Обновляем файл блокировки
    moveLinesBetweenCSVs(currentTable);  // Перемещаем строки между CSV файлами
    rewriteAllCSVFiles(currentTable);  // Перезаписываем все CSV файлы
  }
}

// Метод для проверки условий в группе условий
bool CONFIG::checkAndConditionsAcrossTables(const LinkedList<string>& conditionGroup, const LinkedList<string>& row, const string& currentTableName, int currentCSVIndex, int currentRowIndex) {
  bool allConditionsMet = true;  // Флаг для проверки всех условий в группе

  for (int m = 0; m < conditionGroup.getSize(); ++m) {
    string condition = conditionGroup[m];
    size_t eqPos = condition.find('=');

    if (eqPos != string::npos) {
      string leftPart = condition.substr(0, eqPos);  // Левая часть до '='
      string rightPart = condition.substr(eqPos + 1);  // Правая часть после '='

      // Удаляем пробелы
      leftPart.erase(remove(leftPart.begin(), leftPart.end(), ' '), leftPart.end());
      rightPart.erase(remove(rightPart.begin(), rightPart.end(), ' '), rightPart.end());

      // Обработка левой части (имя таблицы и колонка)
      size_t leftDotPos = leftPart.find('.');
      string leftTableName = leftPart.substr(0, leftDotPos);  // Имя таблицы слева
      string leftColumn = leftPart.substr(leftDotPos + 1);  // Имя колонки слева

      // Если запросы в одном блоке не относятся к одной таблице
      if (leftTableName != currentTableName) {
        allConditionsMet = false;  // все не относится к одной таблице
        break;
      }

      // Поиск индекса колонки в текущей таблице
      Table& leftTable = searchTable(leftTableName);
      int leftColumnIndex = findColumnIndex(leftTable.csv[0], leftColumn);
      if (leftColumnIndex == -1) {
        cerr << "Ошибка: Колонка " << leftColumn << " не найдена в таблице " << leftTableName << endl;
        allConditionsMet = false;
        break;
      }

      // Обработка правой части
      bool isRightPartColumn = rightPart.find('.') != string::npos;
      if (isRightPartColumn) {
        // Правая часть - это колонка другой таблицы
        size_t rightDotPos = rightPart.find('.');
        string rightTableName = rightPart.substr(0, rightDotPos);
        string rightColumn = rightPart.substr(rightDotPos + 1);

        // Поиск таблицы для правой части
        Table& rightTable = searchTable(rightTableName);
        int rightColumnIndex = findColumnIndex(rightTable.csv[0], rightColumn);
        if (rightColumnIndex == -1) {
          cerr << "Ошибка: Колонка " << rightColumn << " не найдена в таблице " << rightTableName << endl;
          allConditionsMet = false;
          break;
        }

        // Проверка наличия строки с данным индексом в правой таблице
        if (currentCSVIndex >= rightTable.csv.getSize()) {
          allConditionsMet = false;
          break;
        }

        // Проверка наличия строки с данным индексом в правой таблице
        if (currentRowIndex >= rightTable.csv[currentCSVIndex].line.getSize()) {
          allConditionsMet = false;
          break;
        }

        // Сравнение значений из обеих таблиц для текущей строки
        string leftValue = row[leftColumnIndex];
        string rightValue = rightTable.csv[currentCSVIndex].line[currentRowIndex][rightColumnIndex];

        if (leftValue != rightValue) {
          allConditionsMet = false;
          break;
        }

      } else {
        // Правая часть - это константа
        string value = rightPart;

        // Сравнение значения с колонкой текущей строки
        if (row[leftColumnIndex] != value) {
          allConditionsMet = false;
          break;
        }
      }
    }
  }

  return allConditionsMet;
}

// Метод для поиска индекса колонки в CSV файле
int CONFIG::findColumnIndex(const CSVFILE& csv, const string& columnName) {
  for (int colIdx = 0; colIdx < csv.columns.getSize(); ++colIdx) {
    if (csv.columns[colIdx] == columnName) {
      return colIdx;
    }
  }
  return -1;  // Если колонка не найдена
}

// Метод для удаления пробелов по краям строки
string CONFIG::trim(const string& str) {
  const char* whitespace = " \t\n\r";
  size_t start = str.find_first_not_of(whitespace);
  size_t end = str.find_last_not_of(whitespace);

  if (start == string::npos || end == string::npos) {
    return "";  // Пустая строка, если только пробелы
  }

  return str.substr(start, end - start + 1);
}

// Метод для поиска таблицы по имени
Table& CONFIG::searchTable(const string& TableName) {
  for (auto& table : structure) {
    if (table.tableName == TableName) {
      return table;
    }
  }

  throw runtime_error("Table not found: " + TableName);
}

// Метод для обновления файла последовательности первичных ключей
void CONFIG::updatePkSeqence(Table& table) {
  ofstream out(table.pathTable + "/" + table.tableName + "_pk_seqence");
  if (out.is_open()) {
    out << table.pk_sequence;
  } else {
    cerr << "Ошибка с файлом: " << table.tableName + "_pk_seqence" << endl;
  }
}

// Метод для обновления CSV файла
void CONFIG::updateCSVFile(Table& table) {
  // Путь к файлу
  if (table.countCSVFile == 0) {
    table.countCSVFile++;
  }

  string csvFilePath = table.pathTable + "/" + to_string(table.countCSVFile) + ".csv";
  ofstream out(csvFilePath, ios::app);  // Открываем файл в режиме добавления

  if (out.is_open()) {
    // Записываем строки
    for (const auto& line : table.csv.back().line) {
      for (size_t j = 0; j < line.getSize(); ++j) {
        out << line[j];
        if (j < line.getSize() - 1) {
          out << ",";
        }
      }
      out << endl;
    }

    out.close();
  } else {
    cerr << "Ошибка при открытии " << csvFilePath << endl;
  }
}

// Метод для обновления файла блокировки
void CONFIG::updateLock(Table& table) {
  ofstream out(table.pathTable + "/" + table.tableName + "_lock");
  if (out.is_open()) {
    out << table.lock;
  } else {
    cerr << "Ошибка с файлом: " << table.tableName + "_lock" << endl;
  }
}

// Метод для перемещения строк между CSV файлами
void CONFIG::moveLinesBetweenCSVs(Table& table) {
  for (size_t i = 0; i < table.csv.getSize(); ++i) {
    // Проверяем, заполнен ли текущий файл
    while (i < table.csv.getSize() && table.csv[i].line.getSize() < table.tuplesLimit) {
      // Если есть следующий файл, переносим строки
      if (i + 1 < table.csv.getSize() && !table.csv[i + 1].line.empty()) {
        size_t linesToMove = table.tuplesLimit - table.csv[i].line.getSize();

        for (size_t j = 0; j < linesToMove && !table.csv[i + 1].line.empty(); ++j) {
          table.csv[i].line.push_back(table.csv[i + 1].line.front());
          table.csv[i + 1].line.erase(0);
        }
      } else {
        break;
      }
    }
  }
}

// Метод для перезаписи всех CSV файлов
void CONFIG::rewriteAllCSVFiles(Table& table) {
  if (table.csv.getSize() == 0 || table.csv[0].line.getSize() == 0) {
    table.countCSVFiles();

    for (int i = 0; i <= table.countCSVFile; i++) {
      fs::path pathCsvFile = fs::path("..") / schemaName / table.tableName / (to_string(i) + ".csv");
      remove(pathCsvFile);
    }
    table.countCSVFile = 0;

    return;
  }

  for (int i = 0; i < table.csv.getSize(); ++i) {
    rewriteFil(table, i);
  }

  while (table.csv.back().line.getSize() == 0) {
    fs::path pathCsvFile = fs::path("..") / schemaName / table.tableName / table.csv.back().csvName;

    if (fs::remove(pathCsvFile)) {
      table.countCSVFile--;
      table.csv.erase(table.csv.getSize() - 1);
    } else {
      cerr << "Не удалось удалить файл " << pathCsvFile << endl;
      break;
    }
  }
}

// Метод для перезаписи конкретного CSV файла
void CONFIG::rewriteFil(Table& table, int numberCsv) {
  // Путь к файлу
  string csvFilePath = table.pathTable + "/" + table.csv[numberCsv].csvName;
  ofstream out(csvFilePath);

  if (out.is_open()) {
    // Записываем заголовки столбцов только один раз из текущего CSV
    const CSVFILE& currentCSV = table.csv[numberCsv];

    // Записываем заголовки столбцов
    for (size_t i = 0; i < currentCSV.columns.getSize(); ++i) {
      out << currentCSV.columns[i];
      if (i < currentCSV.columns.getSize() - 1) {
        out << ",";
      }
    }
    out << endl;

    // Записываем строки
    for (const auto& line : currentCSV.line) {
      for (size_t j = 0; j < line.getSize(); ++j) {
        out << line[j];
        if (j < line.getSize() - 1) {
          out << ",";
        }
      }
      out << endl;
    }

    out.close();
  } else {
    cerr << "Ошибка при открытии " << csvFilePath << endl;
  }
}

void CONFIG::rewriteFil(string& fileName, LinkedList<LinkedList<string>> row) {
  // Путь к файлу

  fs::path FilePath = fs::path("..") / (fileName + ".csv");
  ofstream out(FilePath);

  if (out.is_open()) {
    // Записываем строки
    for (const auto& line : row) {
      for (size_t j = 0; j < line.getSize(); ++j) {
        out << line[j];
        if (j < line.getSize() - 1) {
          out << ",";
        }
      }
      out << endl;
    }

    out.close();
  } else {
    cerr << "Ошибка при открытии файла: " << fileName << endl;
  }
}


// Метод для вывода информации о схеме
void CONFIG::printInfo() const {
  cout << "Schema Name: " << schemaName << endl;
  cout << "Tuples Limit: " << tuplesLimit << endl;
  cout << "Path: " << pathSchema << endl;
  cout << "Tables: " << structure.getSize() << endl;

  cout << endl;

  for (size_t i = 0; i < structure.getSize(); ++i) {
    cout << "Table " << (i + 1) << ": " << structure[i].tableName << endl;
    cout << "Count CSV file: " << structure[i].countCSVFile << endl;
    cout << "Pk_sequence: " << structure[i].pk_sequence << endl;
    cout << "PathTable: " << structure[i].pathTable << endl;

    // Вывод колонок таблицы
    cout << "Columns: ";
    structure[i].csv[0].columns.print();

    // Вывод строк данных
    for (auto& csv : structure[i].csv) {
      for (auto line : csv.line) {
        line.print();
      }
    }

    cout << endl;
  }
  cout << endl;
}

// Метод для выполнения перекрестного соединения
LinkedList<string> CONFIG::crossJoin(LinkedList<string>& first, LinkedList<string>& second) {
  LinkedList<string> result;  // Результирующий массив строк

  // Проходим по каждому элементу из первого массива
  for (size_t i = 0; i < first.getSize(); ++i) {
    const string& firstValue = first[i];

    // Для каждого элемента из первого массива, проходим по элементам второго
    for (size_t j = 0; j < second.getSize(); ++j) {
      const string& secondValue = second[j];

      // Создаем строку, соединяя элементы из первого и второго массивов
      string combinedRow = firstValue + ", " + secondValue;

      result.push_back(combinedRow);  // Добавляем результат в массив строк
    }
  }

  return result;  // Возвращаем результат
}