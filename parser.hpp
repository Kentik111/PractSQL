#ifndef PARSER_HPP
#define PARSER_HPP

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>

#include "config.hpp"
#include "linkedlist.hpp"
using namespace std;

class Pars_SQL {
 public:
  Pars_SQL(CONFIG& configuration);

  void parse(const string& query);

 private:
  CONFIG& config;

  void handleInsert(istringstream& stream);

  void handleSelect(istringstream& stream);

  void handleDelete(istringstream& stream);

  LinkedList<LinkedList<string>> parseConditions(string& query);

  LinkedList<string> parseValues(const string& valuesList);
};

#include "../source/parser.cpp"

#endif
