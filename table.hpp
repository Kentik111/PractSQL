#ifndef TABLE_H
#define TABLE_H

#include <iostream>
#include <string>

#include "../headers/linkedlist.hpp"
#include "../headers/csvfile.hpp"

using namespace std;

class Table {
 public:
  string tableName;
  string pathTable;
  LinkedList<CSVFILE> csv;
  int tuplesLimit;
  int countCSVFile;
  int pk_sequence;
  int lock;  // 0-открыт 1-закрыт

  // Конструктор по умолчанию
  Table();

  // Конструктор с параметрами
  Table(const string& name, const LinkedList<string>& cols);

  // Конструктор копирования
  Table(const Table& other);

  // Оператор присваивания
  Table& operator=(const Table& other);

  void countCSVFiles();  // Подсчитывает количество CSV-файлов
  void readLockFile();  // Читает файл блокировки
  void readPKSequenceFile();  // Читает файл последовательности pk_sequence
  int counterAllLine();

  // Метод для получения массива значений определенной колонки
  LinkedList<string> getColumnValues(const string& columnName);

 private:
  // Метод для поиска индекса колонки по имени
  int findColumnIndex(const CSVFILE& csv, const string& columnName) const;
};

#include "../source/table.cpp"

#endif
