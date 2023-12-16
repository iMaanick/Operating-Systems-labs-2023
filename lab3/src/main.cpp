#include <iostream>
#include <stdlib.h>

#include "test.h"

using namespace std;


void RunTests (SetType type, int argc, char* argv[]) {
   cout << "START RUNNING TESTS:" << endl;

   int threadsCnt = atoi(argv[1]);
   int recordsCnt = atoi(argv[2]);

   cout << "[TEST_WRITERS] Started..." << endl;
   if (RunTestWriters(SetType::FINE_GRAINED, threadsCnt, recordsCnt))
      cout << "[TEST_WRITERS] Succsess!" << endl;
   else
      cout << "[TEST_WRITERS] Failed!" << endl;


   cout << "[TEST_READERS] Started..." << endl;
   if (RunTestReaders(SetType::FINE_GRAINED, threadsCnt, recordsCnt))
      cout << "[TEST_READERS] Succsess!" << endl;
   else
      cout << "[TEST_READERS] Failed!" << endl;


   cout << "[TEST_COMMON] Started..." << endl;
   if (RunTestCommon(SetType::FINE_GRAINED, threadsCnt, recordsCnt, threadsCnt, recordsCnt))
      cout << "[TEST_COMMON] Succsess!" << endl;
   else
      cout << "[TEST_COMMON] Failed!" << endl;

   RunTestTimes(SetType::FINE_GRAINED, threadsCnt, recordsCnt, threadsCnt, recordsCnt);
}


int main (int argc, char* argv[]) {
   if (argc != 3) {
      cout << "Arg1: threads count" << endl;
      cout << "Arg2: records count" << endl;
      return 1;
   }

   cout << "[TEST] Fine grained sync set tests" << endl;
   RunTests(SetType::FINE_GRAINED, argc, argv);
   cout << "[TEST] Lazy set sync tests" << endl;
   RunTests(SetType::LAZY, argc, argv);

   return 0;
}
  


   
