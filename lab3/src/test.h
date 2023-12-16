#ifndef _TEST_H_
#define _TEST_H_


enum class SetType {
   FINE_GRAINED,
   LAZY
};


bool RunTestWriters (SetType type, int threadsCount, int wRecordsCount);
bool RunTestReaders (SetType type, int threadsCount, int rRecordsCount);
bool RunTestCommon (SetType type, int wThreadsCount, int wRecordsCount, int rThreadsCount, int rRecordsCount);
bool RunTestTimes (SetType type, int wThreadsCount, int wRecordsCount, int rThreadsCount, int rRecordsCount);

#endif
