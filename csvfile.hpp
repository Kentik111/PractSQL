#ifndef CSVFILE_H
#define CSVFILE_H

#include <iostream>
#include <string>

#include "../headers/linkedlist.hpp"

using namespace std;

class CSVFILE {
 public:
  string csvName;
  LinkedList<string> columns;
  LinkedList<LinkedList<string>> line;

  CSVFILE();

  CSVFILE(const string name, const LinkedList<string> cols);
  CSVFILE(const string& name);

  int countLine();

  string getColumnValue(const string& columnName, int rowIndex) const;
};

#include "../source/csvfile.cpp"

#endif
