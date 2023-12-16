#include <pthread.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <string>
#include <random>
#include <time.h>

#include "test.h"
#include "../headers/fine_grained_sync_set.h"
#include "../headers/lazy_sync_set.h"

#define DBG_LOG_ENABLED true

using namespace std;

typedef void (*FillDataFunc)(vector<int>::iterator, vector<int>::iterator, int arg);

struct ThreadTestData {
   Set<int>* pSet;

   vector<int>* values;
   int start;
   int count;
};


class Timer {
public:
   void Start () {
      clock_gettime(CLOCK_MONOTONIC, &time);
      ios_base::sync_with_stdio(false);
   } 
   
   double Duration () {
      struct timespec curTime;
      clock_gettime(CLOCK_MONOTONIC, &curTime);
      double timeTaken = (curTime.tv_sec - time.tv_sec) * 1e9;
      return (timeTaken + (curTime.tv_nsec - time.tv_nsec)) * 1e-9;
   }

private:
   struct timespec time;
};


void iota (vector<int>::iterator f, vector<int>::iterator e, int dummy) {
   std::iota(f, e, 0);
}


void shuffle (vector<int>::iterator f, vector<int>::iterator e, int dummy) {
   std::iota(f, e, 0);
   std::shuffle(f, e, std::default_random_engine(0));
}


void fixed (vector<int>::iterator first, vector<int>::iterator end, int thCount) {
   int i = 0, k = 0;
   int size = end - first;
   int recordsCount = size / thCount;
   for (vector<int>::iterator it = first; it != end; it++) {
      if (i / thCount == recordsCount) {
         k++;
         i = k;
      }
      (*it) = i;
      i += thCount;
   }
}


void* WriteTask (void* args) {
   ThreadTestData* thData = (ThreadTestData*)args;
   for (int i = 0; i < thData->count; i++) { 
      thData->pSet->Add(thData->values->at(thData->start + i));
   }
   return args;
}


void* ReadTask (void* args) {
   ThreadTestData* thData = (ThreadTestData*)args;
    
   for (int i = 0; i < thData->count; i++) { 
      thData->pSet->Remove(thData->values->at(thData->start + i));
   }
   return args;
}


void* CommonReadTask (void* args) {
   ThreadTestData* thData = (ThreadTestData*)args;
    
   for (int i = 0; i < thData->count; i++) { 
      if (!thData->pSet->Remove(thData->values->at(thData->start + i))) {
         i--;
         sched_yield();
      }
   }
   return args;
}


void JoinThreads (vector<pthread_t> threads) {
   for (size_t i = 0; i < threads.size(); i++) {
      pthread_join(threads[i], NULL);
   }
}


bool CreateThreads (vector<pthread_t>& threads, void *(*start_routine) (void *), vector<ThreadTestData>* tvDatas) {
   for (size_t i = 0; i < threads.size(); i++) {
      int res = pthread_create(&threads[i], NULL, start_routine, &tvDatas->at(i));
      if (res) {
         printf("[ERROR] Failes while create %lu thread, error number is %d\n", i, res);        
         return false;
      }
   }
   return true;
}


Set<int>* CreateSet (SetType type) {
   switch (type) {
      case SetType::FINE_GRAINED:
         return new(nothrow) FineGrainedSyncSet<int>();
      case SetType::LAZY:
         return new(nothrow) LazySyncSet<int>();
      default:
         return NULL;
   }  
}


void InitThreadsTestDatas (vector<ThreadTestData>* pThDatas, Set<int>* pSet, vector<int>* values, int recordsCount) {
   for (size_t i = 0; i < pThDatas->size(); i++) {
      ThreadTestData& data = pThDatas->at(i);
      data.pSet = pSet;
      data.values = values;
      data.start = i * recordsCount;
      data.count = recordsCount;
   }
}


bool CheckSetValuesWTest (Set<int>* pSet, const vector<int>* values) {
   for (size_t i = 0; i < values->size(); i++) {
      if (!pSet->Remove(values->at(i))) {
         printf("[ERROR] Writer put %i, but it wasn't added\n", values->at(i));
         return false;
      }
   }

   if (pSet->IsEmpty())
      return true;
   return false;
}


bool CheckSetValuesRTest (Set<int>* pSet) {
   if (pSet->IsEmpty())
      return true;
   return false;
}


bool RunTestWritersInternal (SetType type, int threadsCount, int wRecordsCount, FillDataFunc fillFunc, double* time) {
   bool res = false;

   vector<pthread_t> threads(threadsCount);

   vector<ThreadTestData>* pThDatas;
   Set<int>* pSet;
   vector<int>* values;
   
   values = new (nothrow) vector<int>;  
   pThDatas = new (nothrow) vector<ThreadTestData>;  
   
   values->resize(threadsCount * wRecordsCount);
   pThDatas->resize(threadsCount);

   fillFunc(values->begin(), values->end(), threadsCount);

   pSet = CreateSet(type);
 
   InitThreadsTestDatas(pThDatas, pSet, values, wRecordsCount);
   
   Timer timer;
   timer.Start();
   CreateThreads(threads, WriteTask, pThDatas);  
   JoinThreads(threads);
   if (time)
      *time = timer.Duration();
 
   res = CheckSetValuesWTest(pSet, values);
   
   delete values;
   delete pThDatas;
   delete pSet;

   return res;
}


bool RunTestWriters (SetType type, int threadsCount, int wRecordsCount) {   
   return RunTestWritersInternal(type, threadsCount, wRecordsCount, iota, NULL);
}


bool RunTestReadersInternal (SetType type, int threadsCount, int rRecordsCount, FillDataFunc fillFunc, double *time) {
   bool res = false;

   vector<pthread_t> threads(threadsCount);

   vector<ThreadTestData>* pThDatas;
   Set<int>* pSet;
   vector<int>* values;
   
   values = new (nothrow) vector<int>;  
   pThDatas = new (nothrow) vector<ThreadTestData>;  
   
   values->resize(threadsCount * rRecordsCount);
   pThDatas->resize(threadsCount);


   fillFunc(values->begin(), values->end(), threadsCount);

   pSet = CreateSet(type);

   for (size_t i = 0; i < values->size(); i++) {
      pSet->Add(values->at(i));
   }

   InitThreadsTestDatas(pThDatas, pSet, values, rRecordsCount);
   Timer timer;
   timer.Start();
   CreateThreads(threads, ReadTask, pThDatas);
   JoinThreads(threads);
   if (time)
      *time = timer.Duration();
  
   res = CheckSetValuesRTest(pSet);
   
   delete values;
   delete pThDatas;
   delete pSet;

   return res;
}


bool RunTestReaders (SetType type, int threadsCount, int rRecordsCount) {
   return RunTestReadersInternal(type, threadsCount, rRecordsCount, iota, NULL);
}


bool RunTestCommonInternal (SetType type, int wThreadsCount, int wRecordsCount, int rThreadsCount, int rRecordsCount, FillDataFunc fillFunc, double *time) {
   bool res = false;

   vector<pthread_t> wThreads(wThreadsCount);
   vector<pthread_t> rThreads(rThreadsCount);

   vector<ThreadTestData>* pWThDatas;
   vector<ThreadTestData>* pRThDatas;
    
   Set<int>* pSet;
   vector<int>* wValues;
   vector<int>* rValues;
   
   wValues = new (nothrow) vector<int>;  
   rValues = new (nothrow) vector<int>;  
   
   pWThDatas = new (nothrow) vector<ThreadTestData>;  
   pRThDatas = new (nothrow) vector<ThreadTestData>;  
   
   wValues->resize(wThreadsCount * wRecordsCount);
   rValues->resize(rThreadsCount * rRecordsCount);
    
   pWThDatas->resize(wThreadsCount);
   pRThDatas->resize(rThreadsCount);

   fillFunc(wValues->begin(), wValues->end(), wThreadsCount);
  
   *rValues = *wValues;
   
   pSet = CreateSet(type);

   InitThreadsTestDatas(pWThDatas, pSet, wValues, wRecordsCount);
   InitThreadsTestDatas(pRThDatas, pSet, rValues, rRecordsCount);

   Timer timer;
   timer.Start();      
   CreateThreads(wThreads, WriteTask, pWThDatas);
   CreateThreads(rThreads, CommonReadTask, pRThDatas);
   JoinThreads(wThreads);
   JoinThreads(rThreads);
   if (time)
      *time = timer.Duration();

  
   res = CheckSetValuesRTest(pSet);
   
   delete wValues;
   delete rValues;
   delete pWThDatas;
   delete pRThDatas;
   delete pSet;

   return res;
}


bool RunTestCommon (SetType type, int wThreadsCount, int wRecordsCount, int rThreadsCount, int rRecordsCount) {
   return RunTestCommonInternal(type, wThreadsCount, wRecordsCount, rThreadsCount, rRecordsCount, iota, NULL);
}


string GetResultString (bool res) {
   if (res)
      return "Succsess";
   return "Failed";
}


string GetTypeString (SetType type) {
   if (type == SetType::FINE_GRAINED) {
      return "Fine Gr.\t";
   } 
   return "Lazy set";
}


bool RunTestTimes (SetType type, int wThreadsCount, int wRecordsCount, int rThreadsCount, int rRecordsCount) {
   double ws = 0, rs = 0, cs = 0, wf = 0, rf = 0, cf = 0;
   bool resWs = false, resRs = false, resCs = false, resWf = false, resRf = false, resCf = false;
  
   cout.precision(10);
   cout.setf(ios::fixed);

   
   cout << "TEST\t\t\t\t\t" << "TIME\t\t\t" << "RESULT" << endl;
   
   resWs = RunTestWritersInternal(type, wThreadsCount, wRecordsCount, shuffle, &ws);
   cout << GetTypeString(type) + "write\t" + "(random)\t" << ws << "\t\t" << GetResultString(resWs) << endl;
   
   resWf = RunTestWritersInternal(type, wThreadsCount, wRecordsCount, fixed, &wf);
   cout << GetTypeString(type) + "write\t" + "(fixed) \t" << wf << "\t\t" << GetResultString(resWf) << endl;

   resRs = RunTestReadersInternal(type, rThreadsCount, rRecordsCount, shuffle, &rs);
   cout << GetTypeString(type) + "read\t" + "(random)\t" << rs << "\t\t" << GetResultString(resRs) << endl;
   
   resRf = RunTestReadersInternal(type, rThreadsCount, rRecordsCount, fixed, &rf);
   cout << GetTypeString(type) + "read\t" + "(fixed) \t" << rf << "\t\t" << GetResultString(resRf) << endl;

   resCs = RunTestCommonInternal(type, wThreadsCount, wRecordsCount, rThreadsCount, rRecordsCount, shuffle, &cs);
   cout << GetTypeString(type) + "common\t" + "(random)\t" << cs << "\t\t" << GetResultString(resCs) << endl;
  
   resCf = RunTestCommonInternal(type, wThreadsCount, wRecordsCount, rThreadsCount, rRecordsCount, fixed, &cf);
   cout << GetTypeString(type) + "common\t" + "(fixed) \t" << cf << "\t\t" << GetResultString(resCf) << endl;
   
   return true;
}



