#pragma once
#ifndef _FINE_GRAINED_SYNC_SET_H_
#define _FINE_GRAINED_SYNC_SET_H_

#include "set.h"

template <typename ELEM_T>
class FineGrainedSyncSet : public Set<ELEM_T> {
private:
   struct Node {
      Node (ELEM_T val);
      ~Node ();

      ELEM_T value;
      Node* next;
     
      pthread_mutex_t mutex;
   };

public:
   FineGrainedSyncSet (void);
   ~FineGrainedSyncSet (void);

   virtual bool Add (ELEM_T elem);
   virtual bool Remove (ELEM_T elem);
   virtual bool Contains (ELEM_T elem);

   virtual bool IsEmpty ();

private:
   Node* head;
};

#include "fine_grained_sync_set.hpp"

#endif


