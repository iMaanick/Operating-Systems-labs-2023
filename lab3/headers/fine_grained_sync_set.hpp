#pragma once
#ifndef _FINE_GRAINED_SYNC_SET_HPP_
#define _FINE_GRAINED_SYNC_SET_HPP_

#include <limits>

#define LOCK   pthread_mutex_lock
#define UNLOCK pthread_mutex_unlock


template <typename ELEM_T>
FineGrainedSyncSet<ELEM_T>::Node::Node (ELEM_T val) {
   pthread_mutex_init(&mutex, NULL);
   value = val;
   next = NULL;
}


template <typename ELEM_T>
FineGrainedSyncSet<ELEM_T>::Node::~Node() {
   pthread_mutex_destroy(&mutex);
}


template <typename ELEM_T>
FineGrainedSyncSet<ELEM_T>::FineGrainedSyncSet (void) {
   // dummy nodes
   head = new (std::nothrow) Node(std::numeric_limits<ELEM_T>::min());
   head->next = new (std::nothrow) Node(std::numeric_limits<ELEM_T>::max());
}


template <typename ELEM_T>
FineGrainedSyncSet<ELEM_T>::~FineGrainedSyncSet (void) {
   while (head != nullptr) {
      Node* node = head;
      head = head->next;
      delete node;
   }
}


template <typename ELEM_T>
bool FineGrainedSyncSet<ELEM_T>::Add (ELEM_T val) {
   bool res = false;   
    
   LOCK(&head->mutex);
   LOCK(&head->next->mutex);
   
   Node* prev = head; 
   Node* cur = head->next;
   
   while (cur->next) {
      if (cur->value >= val)
         break;

      UNLOCK(&prev->mutex);
      
      prev = cur;
      cur = cur->next;
      
      LOCK(&cur->mutex);
   }

   if (cur->value > val) {
      Node* node = new (std::nothrow) Node(val);
      node->next = cur;
      prev->next = node;
   } else if (!cur) {
      Node* node = new (std::nothrow) Node(val);
      prev->next = node;
      res = true; 
   }

   UNLOCK(&prev->mutex);
   UNLOCK(&cur->mutex);

   return res;
}


template <typename ELEM_T>
bool FineGrainedSyncSet<ELEM_T>::Remove (ELEM_T val) {
   LOCK(&head->mutex);
   LOCK(&head->next->mutex);
   
   Node* prev = head;
   Node* cur = head->next;

   while (cur && cur->value < val) {
      Node* oldprev = prev;
      prev = cur;
      cur = cur->next;
      UNLOCK(&oldprev->mutex);
      LOCK(&cur->mutex);
   }

   if (cur->value == val) {
      prev->next = cur->next;
      UNLOCK(&prev->mutex);
      UNLOCK(&cur->mutex); 
      delete cur;
      return true;
   }

   UNLOCK(&prev->mutex);
   UNLOCK(&cur->mutex); 
   return false;
}


template <typename ELEM_T>
bool FineGrainedSyncSet<ELEM_T>::Contains (ELEM_T val) {
   bool res = false; 

   LOCK(&head->mutex);
   LOCK(&head->next->mutex);
   
   Node* prev = head;
   Node* cur = head->next;

   while (cur && cur->value < val) {
      Node* oldprev = prev;
      prev = cur;
      cur = cur->next;
      UNLOCK(&oldprev->mutex);
      LOCK(&cur->mutex);
   }

   if (cur->value == val) {
      res = true;
   }

   UNLOCK(&prev->mutex);
   UNLOCK(&cur->mutex); 
   return res;
}


template <typename ELEM_T>
bool FineGrainedSyncSet<ELEM_T>::IsEmpty () {
   return (head->value == std::numeric_limits<ELEM_T>::min() && head->next->value == std::numeric_limits<ELEM_T>::max());
}

#endif
