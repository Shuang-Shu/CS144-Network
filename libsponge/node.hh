#ifndef NODE_HH
#define NODE_HH

#include <memory>

using namespace std;

template <class T>
class Node {
  public:
    T val;
    shared_ptr<Node> next;
    shared_ptr<Node> prev;
    Node(T v) : val(v), next(nullptr), prev(nullptr) {}
};

template <class T>
class LinkedBuffer {
  public:
    shared_ptr<Node<T>> head;
    shared_ptr<Node<T>> tail;
    void addTail(T);
    void removeTail();
    T pop();
    T peek();
};
#endif