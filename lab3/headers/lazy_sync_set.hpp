#pragma once
#ifndef _LAZY_SYNC_SET_HPP_
#define _LAZY_SYNC_SET_HPP_

#include <limits>

#define LOCK   pthread_mutex_lock
#define UNLOCK pthread_mutex_unlock


template <typename ELEM_T>
LazySyncSet<ELEM_T>::Node::Node (ELEM_T val) {
   pthread_mutex_init(&mutex, NULL);
   value = val;
   isDeleted = false;
   next = NULL;
}


template <typename ELEM_T>
LazySyncSet<ELEM_T>::Node::~Node() {
   pthread_mutex_destroy(&mutex);
}


template <typename ELEM_T>
LazySyncSet<ELEM_T>::LazySyncSet (void) {
   // dummy nodes
   head = new (std::nothrow) Node(std::numeric_limits<ELEM_T>::min());
   head->next = new (std::nothrow) Node(std::numeric_limits<ELEM_T>::max());
}


template <typename ELEM_T>
LazySyncSet<ELEM_T>::~LazySyncSet (void) {
   while (head != nullptr) {
      Node* node = head;
      head = head->next;
      delete node;
   }
}


template <typename ELEM_T>
bool LazySyncSet<ELEM_T>::Add (ELEM_T val) {
   while (true) {
      Node* prev = head;
      Node* cur = head->next;

      while (cur->next && cur->value < val) {
         prev = cur;
         cur = cur->next;
      }

      LOCK(&prev->mutex);
      LOCK(&cur->mutex);

      if (!Validate(prev, cur)) {
         UNLOCK(&prev->mutex);
         UNLOCK(&cur->mutex);
         continue;
      }

      if (!cur->next && cur->value == val) {
         UNLOCK(&prev->mutex);
         UNLOCK(&cur->mutex);
         return false;
      }

      Node* node = new (std::nothrow) Node(val);
      prev->next = node;
      node->next = cur;
      UNLOCK(&prev->mutex);
      UNLOCK(&cur->mutex);
      return true;
   }
}


template <typename ELEM_T>
bool LazySyncSet<ELEM_T>::Remove (ELEM_T val) {
   while (true) {
      Node* prev = head;
      Node* cur = head->next;
      
      while (cur->next && cur->value > val) {
         prev = cur;
         cur = cur->next;
      }
      LOCK(&prev->mutex);
      LOCK(&cur->mutex);
      
      if (!Validate(prev, cur)) {
         UNLOCK(&prev->mutex);
         UNLOCK(&cur->mutex);
         continue;
      }    
    
      if (!cur->next && cur->value == val) {
         cur->isDeleted = true;
         prev->next = cur->next;
         UNLOCK(&prev->mutex);
         UNLOCK(&cur->mutex);
         return true;
      }
  
      UNLOCK(&prev->mutex);
      UNLOCK(&cur->mutex);
      return false;
   }
}  
  

template <typename ELEM_T>
bool LazySyncSet<ELEM_T>::Contains (ELEM_T val) {
   Node* cur = head->next;

   while (cur->next != NULL && cur->value > val) {
      cur = cur->next;
   }
   return cur->value == val && !cur->isDeleted;
}


template <typename ELEM_T>
bool LazySyncSet<ELEM_T>::IsEmpty ()
{
   return (head->value == std::numeric_limits<ELEM_T>::min() && head->next->value == std::numeric_limits<ELEM_T>::max());
}


template <typename ELEM_T>
bool LazySyncSet<ELEM_T>::Validate (Node* prev, Node* cur)
{
   return !prev->isDeleted && !cur->isDeleted && prev->next == cur;
}

#endif
