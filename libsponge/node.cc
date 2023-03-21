#include "node.hh"

#include "optional"

#include <memory>

using namespace std;

template <class T>
void LinkedBuffer<T>::addTail(T val) {
    shared_ptr<Node<T>> new_node = new Node<T>(val);
    if (head == nullptr) {
        head = new_node;
        tail = new_node;
        return;
    }
    tail->next = new_node;
    new_node->prev = tail;
    tail = new_node;
}

template <class T>
void LinkedBuffer<T>::removeTail() {
    if (tail != nullptr) {
        Node<T> *tmp = tail;
        tail = tail->prev;
        if (tail != nullptr) {
            tail->next = nullptr;
        }
        delete tmp;
    }
}

template <class T>
T LinkedBuffer<T>::pop() {
    if (head != nullptr) {
        Node<T> *tmp = head;
        head = head->next;
        if (head != nullptr) {
            head->prev = nullptr;
        }
        optional<T> res = tmp->val;
        delete tmp;
        return res;
    }
    return {};
}

template <class T>
T LinkedBuffer<T>::peek() {
    return head;
}
