#pragma once
#ifndef _LAZY_SYNC_SET_H_
#define _LAZY_SYNC_SET_H_

#include "set.h"

template <typename ELEM_T>
class LazySyncSet : public Set<ELEM_T> {
private:
   struct Node {
      Node (ELEM_T val);
      ~Node ();

      ELEM_T value;
      Node* next;
     
      pthread_mutex_t mutex;
      bool isDeleted;
   };

public:
   LazySyncSet (void);
   ~LazySyncSet (void);

   virtual bool Add (ELEM_T elem);
   virtual bool Remove (ELEM_T elem);
   virtual bool Contains (ELEM_T elem);

   virtual bool IsEmpty ();

private:
   bool Validate (Node* prev, Node* cur);
   
   Node* head;
};

#include "lazy_sync_set.hpp"

#endif


