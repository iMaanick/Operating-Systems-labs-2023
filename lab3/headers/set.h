#pragma  once
#ifndef _SET_H_
#define _SET_H_

template <typename ELEM_T>
class Set {
public:
   virtual ~Set () {}
 
   virtual bool Add (ELEM_T elem) = 0;
   virtual bool Remove (ELEM_T elem) = 0;
   virtual bool Contains (ELEM_T elem) = 0;

   virtual bool IsEmpty () = 0;
};

#endif
