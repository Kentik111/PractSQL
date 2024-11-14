#ifndef LINKEDLIST_HPP
#define LINKEDLIST_HPP

#include <iostream>
#include <stdexcept>

template <typename T>
class LinkedList {
 private:
  struct Node {
    T data;
    Node* next;
    Node* prev;

    Node(const T& value) : data(value), next(nullptr), prev(nullptr) {}
  };

  Node* head;  // Указатель на начало списка
  Node* tail;  // Указатель на конец списка
  size_t size;  // Текущий размер (количество элементов)

 public:
  LinkedList();
  ~LinkedList();

  void push_back(const T& value);  // Добавление элемента в конец
  void insert_beginning(const T& value);  // Вставка элемента в начало
  T& operator[](size_t index);  // Оператор доступа по индексу
  const T& operator[](size_t index) const;
  size_t getSize() const;  // Получение текущего размера
  void clear();            // Очистка списка
  bool empty() const;      // Проверка на пустоту
  T& back();   // Возврат последнего элемента
  T& front();  // Возврат первого элемента
  void erase(size_t index);  // Удаление элемента по индексу
  LinkedList<T> copy() const;

  bool contains(const T& value) const;  // Проверка на наличие элемента

  void print() const;  // Вывод элементов списка

  // Итераторы
  class Iterator {
   public:
    Iterator(Node* ptr) : current(ptr) {}

    T& operator*() { return current->data; }
    Iterator& operator++() {
      current = current->next;
      return *this;
    }
    Iterator& operator--() {
      current = current->prev;
      return *this;
    }
    bool operator!=(const Iterator& other) const {
      return current != other.current;
    }
    bool operator==(const Iterator& other) const {
      return current == other.current;
    }

   private:
    Node* current;  // Указатель на текущий элемент
  };

  Iterator begin() { return Iterator(head); }
  Iterator end() { return Iterator(nullptr); }

  // Итераторы для константных объектов
  class ConstIterator {
   public:
    ConstIterator(const Node* ptr) : current(ptr) {}

    const T& operator*() const { return current->data; }
    ConstIterator& operator++() {
      current = current->next;
      return *this;
    }
    ConstIterator& operator--() {
      current = current->prev;
      return *this;
    }
    bool operator!=(const ConstIterator& other) const {
      return current != other.current;
    }
    bool operator==(const ConstIterator& other) const {
      return current == other.current;
    }

   private:
    const Node* current;  // Указатель на текущий элемент
  };

  ConstIterator begin() const { return ConstIterator(head); }
  ConstIterator end() const { return ConstIterator(nullptr); }
};

#include "../source/linkedlist.cpp"
#endif
