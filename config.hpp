#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>

#include "table.hpp"
#include "linkedlist.hpp"

using namespace std;
namespace fs = filesystem;

class CONFIG {
 private:
 public:
  string schemaName;
  int tuplesLimit;
  string pathSchema;
  LinkedList<Table> structure;

  CONFIG();
  ~CONFIG(){};

  void readConfig(string PathSchema);
  void createDirectoriesAndFiles();
  void printInfo() const;

  void insertIntoTable(string TableName, LinkedList<string> values);

  void applySelect(const LinkedList<string>& tableNames,
                   const LinkedList<string>& tableColumns);
  void applyWhereConditions(const LinkedList<string>& tableNames,const LinkedList<string>& tableColumns,const LinkedList<LinkedList<string>>& conditional);
  void applyDeleteConditions(const LinkedList<string>& tableNames,const LinkedList<LinkedList<string>>& conditions);

 private:
  Table& searchTable(const string& TableName);
  string trim(const string& str);

  void loadExistingSchemaData(LinkedList<string>& tableNames);
  void loadExistingSchemaData(const LinkedList<string>& tableNames);
  void loadExistingSchemaData(string& tableName);

  void unloadSchemaData(LinkedList<string>& tableNames);
  void unloadSchemaData(const LinkedList<string>& tableNames);
  void unloadSchemaData(string& tableNames);

  LinkedList<string> parseCSVLine(const string& line);

  bool checkAndConditionsAcrossTables(const LinkedList<string>& conditionGroup,const LinkedList<string>& row,const string& currentTableName,int currentCSVIndex, int currentRowIndex);

  
  void processTableWithConditions(const LinkedList<string>& tableNames,const LinkedList<LinkedList<string>>& conditional,function<void(const LinkedList<string>&)> action);


  void processTableWithConditions(const LinkedList<string>& tableNames,const LinkedList<LinkedList<string>>& conditional,const function<void(CSVFILE&, int)>& action);

  int findColumnIndex(const CSVFILE& csv, const string& columnName);

  LinkedList<string> crossJoin(LinkedList<string>& first, LinkedList<string>& second);

  void updatePkSeqence(Table& table);
  void updateCSVFile(Table& table);  
  void updateLock(Table& table);

  void moveLinesBetweenCSVs(Table& table);

  void rewriteAllCSVFiles(Table& table);
  void rewriteFil(Table& table, int numberCsv);  

  void rewriteFil(string& fileName, const LinkedList<string>& columns,const LinkedList<string>& row);
  void rewriteFil(string& fileName,LinkedList<LinkedList<string>> row);  
};
#include "../source/config.cpp"
#endif