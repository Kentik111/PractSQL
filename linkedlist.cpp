#include "../headers/linkedlist.hpp"

// Конструктор по умолчанию
template <typename T>
LinkedList<T>::LinkedList() : head(nullptr), tail(nullptr), size(0) {}

template <typename T>
LinkedList<T>::~LinkedList() {}

// Добавление элемента в конец
template <typename T>
void LinkedList<T>::push_back(const T& value) {
  Node* newNode = new Node(value);
  if (tail) {
    tail->next = newNode;
    newNode->prev = tail;
    tail = newNode;
  } else {
    head = tail = newNode;
  }
  ++size;
}

// Вставка элемента в начало
template <typename T>
void LinkedList<T>::insert_beginning(const T& value) {
  Node* newNode = new Node(value);
  if (head) {
    newNode->next = head;
    head->prev = newNode;
    head = newNode;
  } else {
    head = tail = newNode;
  }
  ++size;
}

// Оператор доступа по индексу
template <typename T>
T& LinkedList<T>::operator[](size_t index) {
  if (index >= size) throw std::out_of_range("Index out of range");

  Node* current = head;
  for (size_t i = 0; i < index; ++i) {
    current = current->next;
  }
  return current->data;
}

template <typename T>
const T& LinkedList<T>::operator[](size_t index) const {
  if (index >= size) throw std::out_of_range("Index out of range");

  Node* current = head;
  for (size_t i = 0; i < index; ++i) {
    current = current->next;
  }
  return current->data;
}

// Получение текущего размера
template <typename T>
size_t LinkedList<T>::getSize() const {
  return size;
}

// Очистка списка
template <typename T>
void LinkedList<T>::clear() {
  while (head) {
    Node* temp = head;
    head = head->next;
    delete temp;
  }
  head = tail = nullptr;
  size = 0;
}

// Проверка на пустоту
template <typename T>
bool LinkedList<T>::empty() const {
  return size == 0;
}

// Возврат последнего элемента
template <typename T>
T& LinkedList<T>::back() {
  if (!tail) throw std::out_of_range("List is empty");
  return tail->data;
}

// Возврат первого элемента
template <typename T>
T& LinkedList<T>::front() {
  if (!head) throw std::out_of_range("List is empty");
  return head->data;
}

// Удаление элемента по индексу
template <typename T>
void LinkedList<T>::erase(size_t index) {
  if (index >= size) throw std::out_of_range("Index out of range");

  Node* current = head;
  for (size_t i = 0; i < index; ++i) {
    current = current->next;
  }

  if (current->prev)
    current->prev->next = current->next;
  else
    head = current->next;

  if (current->next)
    current->next->prev = current->prev;
  else
    tail = current->prev;

  delete current;
  --size;
}

// Проверка на наличие элемента
template <typename T>
bool LinkedList<T>::contains(const T& value) const {
  Node* current = head;
  while (current) {
    if (current->data == value) return true;
    current = current->next;
  }
  return false;
}

// Вывод элементов списка
template <typename T>
void LinkedList<T>::print() const {
  Node* current = head;
  while (current) {
    std::cout << current->data << " ";
    current = current->next;
  }
  std::cout << std::endl;
}

// Метод для создания копии списка
template <typename T>
LinkedList<T> LinkedList<T>::copy() const {
  LinkedList<T> newList;  // Новый объект LinkedList для копии
  Node* current = head;
  while (current) {
    newList.push_back(
        current->data);  // Добавляем каждый элемент в новый список
    current = current->next;
  }
  return newList;
}
