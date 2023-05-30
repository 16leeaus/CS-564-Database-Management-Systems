/*******************************************************
 *
 * Name:       Jacob Dillie
 * Student ID: 9079059573
 * Wisc ID:    dillie@wisc.edu
 *
 * Name:       Ben Milas
 * Student ID: 9080798920
 * Wisc ID:    bmilas@wisc.edu
 *
 * Name:       Austin Lee
 * Student ID: 9073532831
 * Wisc ID:    alee88@wisc.edu
 *
 * Class:      Comp Sci 564
 * Semester:   Spring 2022
 * Project:    Assignment 3
 * File:       main.cpp
 *
 *******************************************************
 *
 * Driver file. Shows how to use File and Page classes.
 * Also contains simple test cases for the B-Tree
 *
 *******************************************************
 *
 * @author See Contributors.txt for code contributors
 * and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences
 * Department, University of Wisconsin-Madison.
 *
 */
//**************************************************

// TODO: Add more tests here (Note that this file has
// code to show how to use the FileScan and BTreeIdnex
// classes.)

#include <vector>
#include "btree.h"
#include "exceptions/bad_opcodes_exception.h"
#include "exceptions/bad_scanrange_exception.h"
#include "exceptions/end_of_file_exception.h"
#include "exceptions/file_not_found_exception.h"
#include "exceptions/index_scan_completed_exception.h"
#include "exceptions/insufficient_space_exception.h"
#include "exceptions/no_such_key_found_exception.h"
#include "exceptions/scan_not_initialized_exception.h"
#include "file_iterator.h"
#include "filescan.h"
#include "page.h"
#include "page_iterator.h"

#define checkPassFail(a, b)                                         \
  {                                                                 \
    if (a == b)                                                     \
      std::cout << "\nTest passed at line no:" << __LINE__ << "\n"; \
    else {                                                          \
      std::cout << "\nTest FAILS at line no:" << __LINE__;          \
      std::cout << "\nExpected no of records:" << b;                \
      std::cout << "\nActual no of records found:" << a;            \
      std::cout << std::endl;                                       \
      exit(1);                                                      \
    }                                                               \
  }

using namespace badgerdb;

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------
std::string relationName = "relA";
// If the relation size is changed then the second parameter 2 checkPassFail may
// need to be changed to number of record that are expected to be found during
// the scan, else tests will erroneously be reported to have failed.
int relationSize = 5000;
std::string intIndexName, doubleIndexName, stringIndexName;

// This is the structure for tuples in the base relation

typedef struct tuple {
  int i;
  double d;
  char s[64];
} RECORD;

PageFile *file1;
RecordId rid;
RECORD record1;
std::string dbRecord1;

BufMgr *bufMgr = new BufMgr(100);

// -----------------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------------

// Insertion methods
void createRelationForward();
void createRelationBackward();
void createRelationRandom();
void createRelationInward();
void createRelationOutward();
void createRelationQuartiles();
void createRelationLarge();

// Universal test method helpers
void intTests();
int intScan(BTreeIndex *index, int lowVal, Operator lowOp, int highVal,
            Operator highOp);
void indexTests();
void deleteRelation();

// Given test methods
void test1();
void test2();
void test3();
// Additional test methods
void test4();
void test5();
void test6();
void test7();
void test8();
void test9();
void test10();
void test11();
void errorTests();
void boundaryTests();
void boundaryTest();

int main(int argc, char **argv) {
  // Clean up from any previous runs that crashed.
  try {
    File::remove(relationName);
  } catch (const FileNotFoundException &) {
  }

  {
    // Create a new database file.
    PageFile new_file = PageFile::create(relationName);

    // Allocate some pages and put data on them.
    for (int i = 0; i < 20; ++i) {
      PageId new_page_number;
      Page new_page = new_file.allocatePage(new_page_number);

      sprintf(record1.s, "%05d string record", i);
      record1.i = i;
      record1.d = (double)i;
      std::string new_data(reinterpret_cast<char *>(&record1), sizeof(record1));

      new_page.insertRecord(new_data);
      new_file.writePage(new_page_number, new_page);
    }
  }
  // new_file goes out of scope here, so file is automatically closed.

  {
    FileScan fscan(relationName, bufMgr);

    try {
      RecordId scanRid;
      while (1) {
        fscan.scanNext(scanRid);
        // Assuming RECORD.i is our key, lets extract the key, which we know is
        // INTEGER and whose byte offset is also know inside the record.
        std::string recordStr = fscan.getRecord();
        const char *record = recordStr.c_str();
        int key = *((int *)(record + offsetof(RECORD, i)));
        std::cout << "Extracted : " << key << std::endl;
      }
    } catch (const EndOfFileException &e) {
      std::cout << "Read all records" << std::endl;
    }
  }
  // filescan goes out of scope here, so relation file gets closed.

  File::remove(relationName);

  test1();
  std::cout << "\n============================================\n\n";
  test2();
  std::cout << "\n============================================\n\n";
  test3();
  std::cout << "\n============================================\n\n";
  test4();
  std::cout << "\n============================================\n\n";
  test5();
  std::cout << "\n============================================\n\n";
  test6();
  std::cout << "\n============================================\n\n";
  test7();
  std::cout << "\n============================================\n\n";
  test8();
  std::cout << "\n============================================\n\n";
  test9();
  std::cout << "\n============================================\n\n";
  test10();
  std::cout << "\n============================================\n\n";
  test11();
  std::cout << "\n============================================\n\n";
  errorTests();

  delete bufMgr;

  return 1;
}

void test1() {
  // Create a relation with tuples valued 0 to relationSize and perform index
  // tests on attributes of all three types (int, double, string)
  std::cout << "---------------------" << std::endl;
  std::cout << "createRelationForward" << std::endl;
  createRelationForward();
  indexTests();
  deleteRelation();
}

void test2() {
  // Create a relation with tuples valued 0 to relationSize in reverse order and
  // perform index tests on attributes of all three types (int, double, string)
  std::cout << "----------------------" << std::endl;
  std::cout << "createRelationBackward" << std::endl;
  createRelationBackward();
  indexTests();
  deleteRelation();
}

void test3() {
  // Create a relation with tuples valued 0 to relationSize in random order and
  // perform index tests on attributes of all three types (int, double, string)
  std::cout << "--------------------" << std::endl;
  std::cout << "createRelationRandom" << std::endl;
  createRelationRandom();
  indexTests();
  deleteRelation();
}

void test4() {
  // Create a relation with tuples valued 0 to relationSize in inward order and
  // perform index tests on attributes of all three types (int, double, string)
  std::cout << "--------------------" << std::endl;
  std::cout << "createRelationInward" << std::endl;
  createRelationInward();
  indexTests();
  deleteRelation();
}

void test5() {
  // Create a relation with tuples valued 0 to relationSize in outward order and
  // perform index tests on attributes of all three types (int, double, string)
  std::cout << "--------------------" << std::endl;
  std::cout << "createRelationOutward" << std::endl;
  createRelationOutward();
  indexTests();
  deleteRelation();
}

void test6() {
  // Create a relation with tuples valued 0 to relationSize in forward order,
  // stratified by and alternating between quartiles
  // perform index tests on attributes of all three types (int, double, string)
  std::cout << "--------------------" << std::endl;
  std::cout << "createRelationQuartiles" << std::endl;
  createRelationQuartiles();
  indexTests();
  deleteRelation();
}

void test7() {
  // Create a relation on a pre-existing index that is being re-opened.
  // perform index tests on attributes of all three types (int, double, string)
  std::cout << "--------------------" << std::endl;
  std::cout << "createRelationPreExistingIndex" << std::endl;
  createRelationForward();
  intTests();
  indexTests();  // btree should still exist here and pass the same tests
  deleteRelation();
}

void test8() {
  // Create a relation with tuples valued 0 to relationSize in random order
  // and perform boundary tests on attributes of all three types
  // (int, double, string)
  std::cout << "--------------------" << std::endl;
  std::cout << "test8 random relation boundary test" << std::endl;
  createRelationRandom();
  boundaryTest();
  deleteRelation();
}

void test9() {
  // Create a relation with tuples valued 0 to relationSize in forward order
  // and perform boundary tests on attributes of all three types
  // (int, double, string)
  std::cout << "--------------------" << std::endl;
  std::cout << "test9 forward relation boundary test" << std::endl;
  createRelationForward();
  boundaryTest();
  deleteRelation();
}

void test10() {
  // Create a relation with tuples valued 0 to relationSize in backward order
  // and perform boundary tests on attributes of all three types
  // (int, double, string)
  std::cout << "--------------------" << std::endl;
  std::cout << "test10 backward relation boundary test" << std::endl;
  createRelationBackward();
  boundaryTest();
  deleteRelation();
}

void test11() {
  // Create a large relation with tuples valued 0 to 700000 in forward order,
  // to force a split on an internal node
  // perform index tests on attributes of all three types (int, double, string)
  std::cout << "--------------------" << std::endl;
  std::cout << "createRelationLarge" << std::endl;
  createRelationLarge();
  indexTests();
  deleteRelation();
}

// -----------------------------------------------------------------------------
// createRelationForward
//
// Order: 0, 1, 2, ... , n-2, n-1
// -----------------------------------------------------------------------------

void createRelationForward() {
  std::vector<RecordId> ridVec;
  // destroy any old copies of relation file
  try {
    File::remove(relationName);
  } catch (const FileNotFoundException &e) {
  }

  file1 = new PageFile(relationName, true);

  // initialize all of record1.s to keep purify happy
  memset(record1.s, ' ', sizeof(record1.s));
  PageId new_page_number;
  Page new_page = file1->allocatePage(new_page_number);

  // Insert a bunch of tuples into the relation.
  for (int i = 0; i < relationSize; i++) {
    sprintf(record1.s, "%05d string record", i);
    record1.i = i;
    record1.d = (double)i;
    std::string new_data(reinterpret_cast<char *>(&record1), sizeof(record1));

    while (1) {
      try {
        new_page.insertRecord(new_data);
        break;
      } catch (const InsufficientSpaceException &e) {
        file1->writePage(new_page_number, new_page);
        new_page = file1->allocatePage(new_page_number);
      }
    }
  }

  file1->writePage(new_page_number, new_page);
}

// -----------------------------------------------------------------------------
// createRelationBackward
//
// Order: n-1, n-2, n-3, ... , 1, 0
// -----------------------------------------------------------------------------

void createRelationBackward() {
  // destroy any old copies of relation file
  try {
    File::remove(relationName);
  } catch (const FileNotFoundException &e) {
  }
  file1 = new PageFile(relationName, true);

  // initialize all of record1.s to keep purify happy
  memset(record1.s, ' ', sizeof(record1.s));
  PageId new_page_number;
  Page new_page = file1->allocatePage(new_page_number);

  // Insert a bunch of tuples into the relation.
  for (int i = relationSize - 1; i >= 0; i--) {
    sprintf(record1.s, "%05d string record", i);
    record1.i = i;
    record1.d = i;

    std::string new_data(reinterpret_cast<char *>(&record1), sizeof(RECORD));

    while (1) {
      try {
        new_page.insertRecord(new_data);
        break;
      } catch (const InsufficientSpaceException &e) {
        file1->writePage(new_page_number, new_page);
        new_page = file1->allocatePage(new_page_number);
      }
    }
  }

  file1->writePage(new_page_number, new_page);
}

// -----------------------------------------------------------------------------
// createRelationRandom
//
// Order: random
// -----------------------------------------------------------------------------

void createRelationRandom() {
  // destroy any old copies of relation file
  try {
    File::remove(relationName);
  } catch (const FileNotFoundException &e) {
  }
  file1 = new PageFile(relationName, true);

  // initialize all of record1.s to keep purify happy
  memset(record1.s, ' ', sizeof(record1.s));
  PageId new_page_number;
  Page new_page = file1->allocatePage(new_page_number);

  // insert records in random order

  std::vector<int> intvec(relationSize);
  for (int i = 0; i < relationSize; i++) {
    intvec[i] = i;
  }

  long pos;
  int val;
  int i = 0;
  while (i < relationSize) {
    pos = random() % (relationSize - i);
    val = intvec[pos];
    sprintf(record1.s, "%05d string record", val);
    record1.i = val;
    record1.d = val;

    std::string new_data(reinterpret_cast<char *>(&record1), sizeof(RECORD));

    while (1) {
      try {
        new_page.insertRecord(new_data);
        break;
      } catch (const InsufficientSpaceException &e) {
        file1->writePage(new_page_number, new_page);
        new_page = file1->allocatePage(new_page_number);
      }
    }

    int temp = intvec[relationSize - 1 - i];
    intvec[relationSize - 1 - i] = intvec[pos];
    intvec[pos] = temp;
    i++;
  }

  file1->writePage(new_page_number, new_page);
}

// -----------------------------------------------------------------------------
// createRelationInward
//
// Order:  0, n-1, 1, n-2, 2, ...
// -----------------------------------------------------------------------------

void createRelationInward() {
  std::vector<RecordId> ridVec;
  // destroy any old copies of relation file
  try {
    File::remove(relationName);
  } catch (const FileNotFoundException &e) {
  }

  file1 = new PageFile(relationName, true);

  // initialize all of record1.s to keep purify happy
  memset(record1.s, ' ', sizeof(record1.s));
  PageId new_page_number;
  Page new_page = file1->allocatePage(new_page_number);

  // Insert a bunch of tuples into the relation.
  for (int i = 0; i < relationSize; i++) {
    sprintf(record1.s, "%05d string record", i);
    if (i % 2 == 0) {
      record1.i = i;
    } else {
      record1.i = relationSize - i;
    }
    record1.d = (double)i;
    std::string new_data(reinterpret_cast<char *>(&record1), sizeof(record1));

    while (1) {
      try {
        new_page.insertRecord(new_data);
        break;
      } catch (const InsufficientSpaceException &e) {
        file1->writePage(new_page_number, new_page);
        new_page = file1->allocatePage(new_page_number);
      }
    }
  }

  file1->writePage(new_page_number, new_page);
}

// -----------------------------------------------------------------------------
// createRelationOutward
//
// Order:  n/2, n/2 + 1, n/2 - 1, n/2 + 2, n/2 - 2, ...
// -----------------------------------------------------------------------------

void createRelationOutward() {
  std::vector<RecordId> ridVec;
  // destroy any old copies of relation file
  try {
    File::remove(relationName);
  } catch (const FileNotFoundException &e) {
  }

  file1 = new PageFile(relationName, true);

  // initialize all of record1.s to keep purify happy
  memset(record1.s, ' ', sizeof(record1.s));
  PageId new_page_number;
  Page new_page = file1->allocatePage(new_page_number);

  // Insert a bunch of tuples into the relation.
  for (int i = 0; i < relationSize; i++) {
    sprintf(record1.s, "%05d string record", i);
    if (i % 2 == 0) {
      record1.i = (relationSize / 2) + (i / 2) - 1;
    } else {
      record1.i = (relationSize / 2) - (i / 2) - 1;
    }
    record1.d = (double)i;
    std::string new_data(reinterpret_cast<char *>(&record1), sizeof(record1));

    while (1) {
      try {
        new_page.insertRecord(new_data);
        break;
      } catch (const InsufficientSpaceException &e) {
        file1->writePage(new_page_number, new_page);
        new_page = file1->allocatePage(new_page_number);
      }
    }
  }

  file1->writePage(new_page_number, new_page);
}

// -----------------------------------------------------------------------------
// createRelationQuartiles
//
// Order:  0, n/4, n/2, 3*n/4, 1, n/4 + 1, n/2 + 1, 3*n/4 + 1, ...
// -----------------------------------------------------------------------------

void createRelationQuartiles() {
  std::vector<RecordId> ridVec;
  // destroy any old copies of relation file
  try {
    File::remove(relationName);
  } catch (const FileNotFoundException &e) {
  }

  file1 = new PageFile(relationName, true);

  // initialize all of record1.s to keep purify happy
  memset(record1.s, ' ', sizeof(record1.s));
  PageId new_page_number;
  Page new_page = file1->allocatePage(new_page_number);

  // Insert a bunch of tuples into the relation.
  for (int i = 0; i < relationSize; i++) {
    sprintf(record1.s, "%05d string record", i);
    record1.i = (((i % 4) * relationSize) / 4) + (i / 4);
    record1.d = (double)i;
    std::string new_data(reinterpret_cast<char *>(&record1), sizeof(record1));

    while (1) {
      try {
        new_page.insertRecord(new_data);
        break;
      } catch (const InsufficientSpaceException &e) {
        file1->writePage(new_page_number, new_page);
        new_page = file1->allocatePage(new_page_number);
      }
    }
  }

  file1->writePage(new_page_number, new_page);
}

// -----------------------------------------------------------------------------
// createRelationLarge
//
// Sequential order, size 700000; causes split in internal node
// -----------------------------------------------------------------------------

void createRelationLarge() {
  // Change Parameters
  const int LARGE_RELATION_SIZE = 700000;

  std::vector<RecordId> ridVec;
  // destroy any old copies of relation file
  try {
    File::remove(relationName);
  } catch (const FileNotFoundException &e) {
  }

  file1 = new PageFile(relationName, true);

  // initialize all of record1.s to keep purify happy
  memset(record1.s, ' ', sizeof(record1.s));
  PageId new_page_number;
  Page new_page = file1->allocatePage(new_page_number);

  // Insert a bunch of tuples into the relation.
  for (int i = 0; i < LARGE_RELATION_SIZE; i++) {
    sprintf(record1.s, "%05d string record", i);
    record1.i = i;
    record1.d = (double)i;
    std::string new_data(reinterpret_cast<char *>(&record1), sizeof(record1));

    while (1) {
      try {
        new_page.insertRecord(new_data);
        break;
      } catch (const InsufficientSpaceException &e) {
        file1->writePage(new_page_number, new_page);
        new_page = file1->allocatePage(new_page_number);
      }
    }
  }

  file1->writePage(new_page_number, new_page);
}

// -----------------------------------------------------------------------------
// BoundaryTests
// -----------------------------------------------------------------------------

void boundaryTests() {
  std::cout << "Create a B+ Tree index on the integer field" << std::endl;
  BTreeIndex index(relationName, intIndexName, bufMgr, offsetof(tuple, i),
                   INTEGER);

  // Run some pass fail checks
  checkPassFail(intScan(&index, 0, GTE, 5000, LT), 5000)
      checkPassFail(intScan(&index, 2000, GTE, 6000, LT), 3000)
          checkPassFail(intScan(&index, 4999, GTE, 6000, LT), 1) checkPassFail(
              intScan(&index, 4999, GTE, 6000, LT), 1)
              checkPassFail(intScan(&index, 5888, GT, 6000, LT), 0)
                  checkPassFail(intScan(&index, -5000, GT, 0, LT), 0)
                      checkPassFail(intScan(&index, -5000, GT, 0, LTE), 1)
                          checkPassFail(intScan(&index, -5000, GT, 10, LTE), 11)
                              checkPassFail(intScan(&index, -5000, GT, 100, LT),
                                            100)
}

// -----------------------------------------------------------------------------
// BoundaryTest (calls and cleans up BoundaryTests)
// -----------------------------------------------------------------------------

void boundaryTest() {
  boundaryTests();
  try {
    File::remove(intIndexName);
  } catch (const FileNotFoundException &e) {
  }
}

// -----------------------------------------------------------------------------
// indexTests
// -----------------------------------------------------------------------------

void indexTests() {
  intTests();
  try {
    File::remove(intIndexName);
  } catch (const FileNotFoundException &e) {
  }
}

// -----------------------------------------------------------------------------
// intTests
// -----------------------------------------------------------------------------

void intTests() {
  std::cout << "Create a B+ Tree index on the integer field" << std::endl;
  BTreeIndex index(relationName, intIndexName, bufMgr, offsetof(tuple, i),
                   INTEGER);
  std::cout << "\nINDEXING COMPLETE!\n\n";
  // run some tests
  checkPassFail(intScan(&index, 25, GT, 40, LT), 14)
      checkPassFail(intScan(&index, 20, GTE, 35, LTE), 16)
          checkPassFail(intScan(&index, -3, GT, 3, LT), 3)
              checkPassFail(intScan(&index, 996, GT, 1001, LT), 4)
                  checkPassFail(intScan(&index, 0, GT, 1, LT), 0) checkPassFail(
                      intScan(&index, 300, GT, 400, LT), 99)
                      checkPassFail(intScan(&index, 3000, GTE, 4000, LT), 1000)
}

int intScan(BTreeIndex *index, int lowVal, Operator lowOp, int highVal,
            Operator highOp) {
  RecordId scanRid;
  Page *curPage;

  std::cout << "Scan for ";
  if (lowOp == GT) {
    std::cout << "(";
  } else {
    std::cout << "[";
  }
  std::cout << lowVal << "," << highVal;
  if (highOp == LT) {
    std::cout << ")";
  } else {
    std::cout << "]";
  }
  std::cout << std::endl;

  int numResults = 0;

  try {
    index->startScan(&lowVal, lowOp, &highVal, highOp);
  } catch (const NoSuchKeyFoundException &e) {
    std::cout << "No Key Found satisfying the scan criteria." << std::endl;
    return 0;
  }

  while (1) {
    try {
      index->scanNext(scanRid);
      bufMgr->readPage(file1, scanRid.page_number, curPage);
      RECORD myRec = *(
          reinterpret_cast<const RECORD *>(curPage->getRecord(scanRid).data()));
      bufMgr->unPinPage(file1, scanRid.page_number, false);

      if (numResults < 5) {
        std::cout << "at:" << scanRid.page_number << "," << scanRid.slot_number;
        std::cout << " -->:" << myRec.i << ":" << myRec.d << ":" << myRec.s
                  << ":" << std::endl;
      } else if (numResults == 5) {
        std::cout << "..." << std::endl;
      }
    } catch (const IndexScanCompletedException &e) {
      break;
    }

    numResults++;
  }

  if (numResults >= 5) {
    std::cout << "Number of results: " << numResults << std::endl;
  }
  index->endScan();
  std::cout << std::endl;

  return numResults;
}

// -----------------------------------------------------------------------------
// errorTests
// -----------------------------------------------------------------------------

void errorTests() {
  {
    std::cout << "Error handling tests" << std::endl;
    std::cout << "--------------------" << std::endl;
    // Given error test

    try {
      File::remove(relationName);
    } catch (const FileNotFoundException &e) {
    }

    file1 = new PageFile(relationName, true);

    // initialize all of record1.s to keep purify happy
    memset(record1.s, ' ', sizeof(record1.s));
    PageId new_page_number;
    Page new_page = file1->allocatePage(new_page_number);

    // Insert a bunch of tuples into the relation.
    for (int i = 0; i < 10; i++) {
      sprintf(record1.s, "%05d string record", i);
      record1.i = i;
      record1.d = (double)i;
      std::string new_data(reinterpret_cast<char *>(&record1), sizeof(record1));

      while (1) {
        try {
          new_page.insertRecord(new_data);
          break;
        } catch (const InsufficientSpaceException &e) {
          file1->writePage(new_page_number, new_page);
          new_page = file1->allocatePage(new_page_number);
        }
      }
    }

    file1->writePage(new_page_number, new_page);

    BTreeIndex index(relationName, intIndexName, bufMgr, offsetof(tuple, i),
                     INTEGER);

    int int2 = 2;
    int int5 = 5;

    // Scan Tests
    std::cout << "Call endScan before startScan" << std::endl;
    try {
      index.endScan();
      std::cout << "ScanNotInitialized Test 1 Failed." << std::endl;
    } catch (const ScanNotInitializedException &e) {
      std::cout << "ScanNotInitialized Test 1 Passed." << std::endl;
    }

    std::cout << "Call scanNext before startScan" << std::endl;
    try {
      RecordId foo;
      index.scanNext(foo);
      std::cout << "ScanNotInitialized Test 2 Failed." << std::endl;
    } catch (const ScanNotInitializedException &e) {
      std::cout << "ScanNotInitialized Test 2 Passed." << std::endl;
    }

    std::cout << "Scan with bad lowOp" << std::endl;
    try {
      index.startScan(&int2, LTE, &int5, LTE);
      std::cout << "BadOpcodesException Test 1 Failed." << std::endl;
    } catch (const BadOpcodesException &e) {
      std::cout << "BadOpcodesException Test 1 Passed." << std::endl;
    }

    std::cout << "Scan with bad highOp" << std::endl;
    try {
      index.startScan(&int2, GTE, &int5, GTE);
      std::cout << "BadOpcodesException Test 2 Failed." << std::endl;
    } catch (const BadOpcodesException &e) {
      std::cout << "BadOpcodesException Test 2 Passed." << std::endl;
    }

    std::cout << "Scan with bad range" << std::endl;
    try {
      index.startScan(&int5, GTE, &int2, LTE);
      std::cout << "BadScanrangeException Test 1 Failed." << std::endl;
    } catch (const BadScanrangeException &e) {
      std::cout << "BadScanrangeException Test 1 Passed." << std::endl;
    }

    deleteRelation();
  }

  try {
    File::remove(intIndexName);
  } catch (const FileNotFoundException &e) {
  }
}

void deleteRelation() {
  if (file1) {
    bufMgr->flushFile(file1);
    delete file1;
    file1 = NULL;
  }
  try {
    File::remove(relationName);
  } catch (const FileNotFoundException &e) {
  }
}
