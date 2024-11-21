#include <fstream>
#include <iostream>
#include <string>

#include "../headers/parser.hpp"
#include "../headers/config.hpp"
#include "../headers/table.hpp"
#include "../headers/linkedlist.hpp"
#include "../library/json.hpp"

using namespace std;

int main() {
  CONFIG config;
  config.readConfig("../schema.json");

  Pars_SQL parser(config);
  string input;
  cout <<"\nSELECT <> FROM <> - выборка\nWHERE и операторы OR , AND - фильтрация\nINSERT INTO - вставка данных в таблицы\nDELETE FROM - удаление данных из таблицы\nEXIT-выход из программы\n";
  cout <<"------------------------------------\n"<<endl;
  while (true) {
    cout << "SQL>";
    getline(cin, input);
    if (input == "EXIT") {
      break;
    }

    parser.parse(input);
    cout << endl;
  }

  return 0;
}
