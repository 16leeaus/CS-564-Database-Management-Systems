/*******************************************************
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
 * Project:    Assignment 2
 * File:       main.cpp
 *******************************************************
 *
 * Driver file. Shows how to use File and Page classes.
 * Also contains simple test cases for the Buffer manager
 *
 */ //**************************************************

#include <stdlib.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <optional>

#include "buffer.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/file_not_found_exception.h"
#include "exceptions/invalid_page_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"
#include "file_iterator.h"
#include "page.h"
#include "page_iterator.h"

#define PRINT_ERROR(str)                            \
  {                                                 \
    std::cerr << "On Line No:" << __LINE__ << "\n"; \
    std::cerr << str << "\n";                       \
    exit(1);                                        \
  }

using namespace badgerdb;

const PageId num = 100;
PageId pid[num], pageno1, pageno2, pageno3, i;
RecordId rid[num], rid2, rid3;
Page *page = NULL, *page2 = NULL, *page3 = NULL;
char tmpbuf[100];
// The one and only buffer manager
std::shared_ptr<BufMgr> bufMgr;
// File pointers used for testing

void test1(File &file1);
void test2(File &file1, File &file2, File &file3);
void test3(File &file4);
void test4(File &file4);
void test5(File &file4);
void test6(File &file1);
void test7(File &file1, File &file2, File &file3);
void test8(File &file3);
void test9(File &file5);
void test10(File &file2);
void test11(File &file5);
void test12(File &file1);
void test13();
void test14(File &file1, File &file2, File &file3);
void test15(File &file2);

// Calls the above tests
void testBufMgr();

int main() {
  // Following code shows how to you File and Page classes

  const std::string filename = "test.db";
  // Clean up from any previous runs that crashed.
  try {
    File::remove(filename);
  } catch (const FileNotFoundException &) {
  }

  {
    // Create a new database file.
    File new_file = File::create(filename);

    // Allocate some pages and put data on them.
    PageId third_page_number;
    for (int i = 0; i < 5; ++i) {
      Page new_page = new_file.allocatePage();
      if (i == 3) {
        // Keep track of the identifier for the third page so we can read
        // it later.
        third_page_number = new_page.page_number();
      }
      new_page.insertRecord("hello!");
      // Write the page back to the file (with the new data).
      new_file.writePage(new_page);
    }

    // Iterate through all pages in the file.
    for (FileIterator iter = new_file.begin(); iter != new_file.end(); ++iter) {
      Page curr_page = (*iter);
      // Iterate through all records on the page.
      for (PageIterator page_iter = curr_page.begin();
           page_iter != curr_page.end(); ++page_iter) {
        std::cout << "Found record: " << *page_iter << " on page "
                  << curr_page.page_number() << "\n";
      }
    }

    // Retrieve the third page and add another record to it.
    Page third_page = new_file.readPage(third_page_number);
    const RecordId &rid = third_page.insertRecord("world!");
    new_file.writePage(third_page);

    // Retrieve the record we just added to the third page.
    std::cout << "Third page has a new record: " << third_page.getRecord(rid)
              << "\n\n";
  }
  // new_file_ptr goes out of scope here, so file is automatically closed.

  // Delete the file since we're done with it.
  File::remove(filename);

  // This function tests buffer manager, comment this line if you don't wish to
  // test buffer manager
  testBufMgr();
}

void testBufMgr() {
  // Create buffer manager
  bufMgr = std::make_shared<BufMgr>(num);

  // Create dummy files
  const std::string filename1 = "test.1";
  const std::string filename2 = "test.2";
  const std::string filename3 = "test.3";
  const std::string filename4 = "test.4";
  const std::string filename5 = "test.5";

  // Clean up from any previous runs that crashed.
  try {
    File::remove(filename1);
    File::remove(filename2);
    File::remove(filename3);
    File::remove(filename4);
    File::remove(filename5);
  } catch (const FileNotFoundException &e) {
  }

  {
    File file1 = File::create(filename1);
    File file2 = File::create(filename2);
    File file3 = File::create(filename3);
    File file4 = File::create(filename4);
    File file5 = File::create(filename5);

    // Test buffer manager
    // Comment tests which you do not wish to run now. Tests are dependent on
    // their preceding tests. So, they have to be run in the following order.
    // Commenting  a particular test requires commenting all tests that follow
    // it else those tests would fail.
    test1(file1);
    test2(file1, file2, file3);
    test3(file4);
    test4(file4);
    test5(file5);
    test6(file1);
    test7(file1, file2, file3);
    test8(file3);
    test9(file5);
    test10(file2);
    test11(file5);
    test12(file1);
    test13();
    test14(file1, file2, file3);
    test15(file2);

    // Close the files by going out of scope
  }

  // Delete files
  File::remove(filename1);
  File::remove(filename2);
  File::remove(filename3);
  File::remove(filename4);
  File::remove(filename5);

  std::cout << "\n"
            << "Passed all tests."
            << "\n";
}

void test1(File &file1) {
  // Allocating pages in a file...
  for (i = 0; i < num; i++) {
    bufMgr->allocPage(file1, pid[i], page);
    sprintf(tmpbuf, "test.1 Page %u %7.1f", pid[i], (float)pid[i]);
    rid[i] = page->insertRecord(tmpbuf);
    bufMgr->unPinPage(file1, pid[i], true);
  }

  // Reading pages back...
  for (i = 0; i < num; i++) {
    bufMgr->readPage(file1, pid[i], page);
    sprintf(tmpbuf, "test.1 Page %u %7.1f", pid[i], (float)pid[i]);
    if (strncmp(page->getRecord(rid[i]).c_str(), tmpbuf, strlen(tmpbuf)) != 0) {
      PRINT_ERROR("ERROR :: CONTENTS DID NOT MATCH");
    }
    bufMgr->unPinPage(file1, pid[i], false);
  }
  std::cout << "Test 1 passed"
            << "\n";
}

void test2(File &file1, File &file2, File &file3) {
  // Writing and reading back multiple files
  // The page number and the value should match

  for (i = 0; i < num / 3; i++) {
    bufMgr->allocPage(file2, pageno2, page2);
    sprintf(tmpbuf, "test.2 Page %u %7.1f", pageno2, (float)pageno2);
    rid2 = page2->insertRecord(tmpbuf);

    long int index = random() % num;
    pageno1 = pid[index];
    bufMgr->readPage(file1, pageno1, page);
    sprintf(tmpbuf, "test.1 Page %u %7.1f", pageno1, (float)pageno1);
    if (strncmp(page->getRecord(rid[index]).c_str(), tmpbuf, strlen(tmpbuf)) !=
        0) {
      PRINT_ERROR("ERROR :: CONTENTS DID NOT MATCH");
    }

    bufMgr->allocPage(file3, pageno3, page3);
    sprintf(tmpbuf, "test.3 Page %u %7.1f", pageno3, (float)pageno3);
    rid3 = page3->insertRecord(tmpbuf);

    bufMgr->readPage(file2, pageno2, page2);
    sprintf(tmpbuf, "test.2 Page %u %7.1f", pageno2, (float)pageno2);
    if (strncmp(page2->getRecord(rid2).c_str(), tmpbuf, strlen(tmpbuf)) != 0) {
      PRINT_ERROR("ERROR :: CONTENTS DID NOT MATCH");
    }

    bufMgr->readPage(file3, pageno3, page3);
    sprintf(tmpbuf, "test.3 Page %u %7.1f", pageno3, (float)pageno3);
    if (strncmp(page3->getRecord(rid3).c_str(), tmpbuf, strlen(tmpbuf)) != 0) {
      PRINT_ERROR("ERROR :: CONTENTS DID NOT MATCH");
    }

    bufMgr->unPinPage(file1, pageno1, false);
  }

  for (i = 0; i < num / 3; i++) {
    bufMgr->unPinPage(file2, i + 1, true);
    bufMgr->unPinPage(file2, i + 1, true);
    bufMgr->unPinPage(file3, i + 1, true);
    bufMgr->unPinPage(file3, i + 1, true);
  }

  std::cout << "Test 2 passed"
            << "\n";
}

void test3(File &file4) {
  try {
    bufMgr->readPage(file4, 1, page);
    PRINT_ERROR(
        "ERROR :: File4 should not exist. Exception should have been "
        "thrown before execution reaches this point.");
  } catch (const InvalidPageException &e) {
  }

  std::cout << "Test 3 passed"
            << "\n";
}

void test4(File &file4) {
  bufMgr->allocPage(file4, i, page);
  bufMgr->unPinPage(file4, i, true);
  try {
    bufMgr->unPinPage(file4, i, false);
    PRINT_ERROR(
        "ERROR :: Page is already unpinned. Exception should have been "
        "thrown before execution reaches this point.");
  } catch (const PageNotPinnedException &e) {
  }

  std::cout << "Test 4 passed"
            << "\n";
}

void test5(File &file5) {
  for (i = 0; i < num; i++) {
    bufMgr->allocPage(file5, pid[i], page);
    sprintf(tmpbuf, "test.5 Page %u %7.1f", pid[i], (float)pid[i]);
    rid[i] = page->insertRecord(tmpbuf);
  }

  PageId tmp;
  try {
    bufMgr->allocPage(file5, tmp, page);
    PRINT_ERROR(
        "ERROR :: No more frames left for allocation. Exception should "
        "have been thrown before execution reaches this point.");
  } catch (const BufferExceededException &e) {
  }

  std::cout << "Test 5 passed"
            << "\n";

  for (i = 1; i <= num; i++) bufMgr->unPinPage(file5, i, true);
}

void test6(File &file1) {
  // flushing file with pages still pinned. Should generate an error
  for (i = 1; i <= num; i++) {
    bufMgr->readPage(file1, i, page);
  }

  try {
    bufMgr->flushFile(file1);
    PRINT_ERROR(
        "ERROR :: Pages pinned for file being flushed. Exception "
        "should have been thrown before execution reaches this point.");
  } catch (const PagePinnedException &e) {
  }

  std::cout << "Test 6 passed"
            << "\n";

  for (i = 1; i <= num; i++) bufMgr->unPinPage(file1, i, true);

  bufMgr->flushFile(file1);
}

void test7(File &file1, File &file2, File &file3) {
  // Allocating pages in a file...
  for (i = 0; i < num; i++) {
    bufMgr->allocPage(file1, pid[i], page);
    sprintf(tmpbuf, "test.1 Page %u %7.1f", pid[i], (float)pid[i]);
    rid[i] = page->insertRecord(tmpbuf);
    bufMgr->unPinPage(file1, pid[i], true);
  }

  // Writing and reading back multiple files
  // after allocated, unpin as we proceed
  for (i = 0; i < num / 3; i++) {
    bufMgr->allocPage(file2, pageno2, page2);
    sprintf(tmpbuf, "test.2 Page %u %7.1f", pageno2, (float)pageno2);
    rid2 = page2->insertRecord(tmpbuf);
    bufMgr->unPinPage(file2, pageno2, true);

    bufMgr->allocPage(file2, pageno2, page2);
    sprintf(tmpbuf, "test.2 Page %u %7.1f", pageno2, (float)pageno2);
    rid2 = page2->insertRecord(tmpbuf);

    bufMgr->readPage(file3, pageno3, page3);
    sprintf(tmpbuf, "test.3 Page %u %7.1f", pageno3, (float)pageno3);
    if (strncmp(page3->getRecord(rid3).c_str(), tmpbuf, strlen(tmpbuf)) != 0) {
      PRINT_ERROR("ERROR :: CONTENTS DID NOT MATCH");
    }

    bufMgr->readPage(file2, pageno2, page2);
    sprintf(tmpbuf, "test.2 Page %u %7.1f", pageno2, (float)pageno2);
    if (strncmp(page2->getRecord(rid2).c_str(), tmpbuf, strlen(tmpbuf)) != 0) {
      PRINT_ERROR("ERROR :: CONTENTS DID NOT MATCH");
    }

    bufMgr->unPinPage(file2, pageno2, true);
    bufMgr->unPinPage(file2, pageno2, false);
    bufMgr->unPinPage(file3, pageno3, true);
  }

  std::cout << "Test 7 passed"
            << "\n";

  bufMgr->flushFile(file3);
  bufMgr->flushFile(file2);
  bufMgr->flushFile(file1);
}

void test8(File &file3) {
  // Reading pages twice to create double pin count
  for (i = 1; i <= num / 3; i++) {
    for (int j = 0; j < 2; j++) {
      bufMgr->readPage(file3, i, page);
    }
  }

  // Unpin once (should cause error)
  for (i = 1; i <= num / 3; i++) bufMgr->unPinPage(file3, i, true);

  try {
    bufMgr->flushFile(file3);
    PRINT_ERROR(
        "ERROR :: Pages pinned for file being flushed. Exception "
        "should have been thrown before execution reaches this point.");
  } catch (const PagePinnedException &e) {
  }

  std::cout << "Test 8 passed"
            << "\n";

  // Unpin again and flush
  for (i = 1; i <= num / 3; i++) bufMgr->unPinPage(file3, i, true);
  bufMgr->flushFile(file3);
}

void test9(File &file5) {
  // test a nearly full allocation (no exception should be thrown)
  for (i = 0; i < num - 1; i++) {
    bufMgr->allocPage(file5, pid[i], page);
    sprintf(tmpbuf, "test.5 Page %u %7.1f", pid[i], (float)pid[i]);
    rid[i] = page->insertRecord(tmpbuf);
  }
  PageId tmp;
  try {
    bufMgr->allocPage(file5, tmp, page);
  } catch (const BufferExceededException &e) {
    PRINT_ERROR(
        "ERROR :: Frame available for allocation. Exception should "
        "not have been thrown.");
  }

  std::cout << "Test 9 passed"
            << "\n";

  for (i = 0; i < num; i++) bufMgr->unPinPage(file5, pid[i], true);
  bufMgr->flushFile(file5);
}

void test10(File &file2) {
  // test that you cannot read disposed pages (also tests that alloc'd pages
  // still get disposed of)
  try {
    bufMgr->allocPage(file2, i, page);
    bufMgr->disposePage(file2, i);
    bufMgr->readPage(file2, i, page);
    PRINT_ERROR(
        "ERROR :: Reading page which was disposed. Exception "
        "should have been thrown before execution reaches this point.");
  } catch (const InvalidPageException &e) {
  }

  std::cout << "Test 10 passed"
            << "\n";
}

void test11(File &file5) {
  // tests that allocating all pages and then unpinning one still allows you to
  // alloc that last page
  for (i = 0; i < num; i++) {
    bufMgr->allocPage(file5, pid[i], page);
    sprintf(tmpbuf, "test.5 Page %u %7.1f", pid[i], (float)pid[i]);
    rid[i] = page->insertRecord(tmpbuf);
  }

  bufMgr->unPinPage(file5, pid[0], false);
  bufMgr->disposePage(file5, pid[0]);

  PageId tmp;
  try {
    bufMgr->allocPage(file5, tmp, page);

  } catch (const BufferExceededException &e) {
    PRINT_ERROR(
        "ERROR :: Frame still available for allocation. Exception should "
        "not have been thrown.");
  }

  for (i = 0; i < num; i++) {
    bufMgr->unPinPage(file5, pid[i], true);
  }

  bufMgr->flushFile(file5);
  std::cout << "Test 11 passed"
            << "\n";
}

void test12(File &file1) {
  // Allocate pages
  for (i = 1; i < num; i++) {
    bufMgr->allocPage(file1, pid[i], page);
  }
  // Allocate page then dispose it (should not have issues)
  PageId pageNum1;
  bufMgr->allocPage(file1, pageNum1, page);
  try {
    bufMgr->disposePage(file1, pageNum1);
  } catch (PagePinnedException &e) {
    PRINT_ERROR(
        "ERROR :: Pages pinned for file being disposed. Exception should not "
        "been thrown since the page is removed entirely.");
  }

  // Attempt to allocate 2 new pages
  // (should cause exception since buffer is full)
  bufMgr->allocPage(file1, pid[0], page);
  PageId tmp;
  try {
    bufMgr->allocPage(file1, tmp, page);
    PRINT_ERROR(
        "ERROR :: No more frames left for allocation. Exception should "
        "have been thrown before execution reaches this point.");
  } catch (const BufferExceededException &e) {
  }

  // Unpin valid pages
  for (i = 0; i < num; i++) {
    try {
      bufMgr->unPinPage(file1, pid[i], true);
    } catch (PageNotPinnedException &e) {
    }
  }

  bufMgr->flushFile(file1);
  std::cout << "Test 12 passed"
            << "\n";
}

void test13() {
  // Test conditions:  File 1 exists, and has at least one page.
  // After test conditions: File 1 will be flushed from buffer.

  File file1ptr;
  PageId firstPageID = 1;

  // Flush all file 1 to get rid of test page
  bufMgr->flushFile(file1ptr);

  // Test nothing happens if page not in buffer
  try {
    bufMgr->unPinPage(file1ptr, firstPageID, false);
  } catch (...) {
    PRINT_ERROR(
        "ERROR :: Page is not in buffer. unPinPage should not throw any "
        "exceptions.");
  }

  // Flush file to attain guaranteed post test state.
  bufMgr->flushFile(file1ptr);

  std::cout << "Test 13 passed\n";
}

void test14(File &file1, File &file2, File &file3) {
  {
    // Allocate pages for three separate files
    for (i = 0; i < num - 2; i += 3) {
      bufMgr->allocPage(file1, pid[i], page);
      bufMgr->allocPage(file2, pid[i + 1], page2);
      bufMgr->allocPage(file3, pid[i + 2], page3);
    }

    // Unpin file 1
    for (i = 0; i < num - 2; i += 3) {
      bufMgr->unPinPage(file1, pid[i], false);
    }

    // Flush file 1 (should not cause exception)
    try {
      bufMgr->flushFile(file1);
    } catch (const PagePinnedException &e) {
      PRINT_ERROR(
          "ERROR :: Pages not pinned for file being flushed. Exception "
          "should not have been thrown.");
    }

    // Flush file 2 (should cause exception)
    try {
      bufMgr->flushFile(file2);
      PRINT_ERROR(
          "ERROR :: Pages pinned for file being flushed. Exception "
          "should have been thrown before execution reaches this point.");
    } catch (const PagePinnedException &e) {
    }

    // Flush file 3 (should cause exception)
    try {
      bufMgr->flushFile(file3);
      PRINT_ERROR(
          "ERROR :: Pages pinned for file being flushed. Exception "
          "should have been thrown before execution reaches this point.");
    } catch (const PagePinnedException &e) {
    }

    // Unpin file 2
    for (i = 0; i < num - 2; i += 3) {
      bufMgr->unPinPage(file2, pid[i + 1], false);
    }

    // Flush file 2 (should not cause exception)
    try {
      bufMgr->flushFile(file2);
    } catch (const PagePinnedException &e) {
      PRINT_ERROR(
          "ERROR :: Pages not pinned for file being flushed. Exception "
          "should not have been thrown.");
    }

    // Flush file 3 (should cause exception)
    try {
      bufMgr->flushFile(file3);
      PRINT_ERROR(
          "ERROR :: Pages pinned for file being flushed. Exception "
          "should have been thrown before execution reaches this point.");
    } catch (const PagePinnedException &e) {
    }

    // Unpin file 3
    for (i = 0; i < num - 2; i += 3) {
      bufMgr->unPinPage(file3, pid[i + 2], false);
    }

    // Flush file 3 (should not cause exception)
    try {
      bufMgr->flushFile(file3);
    } catch (const PagePinnedException &e) {
      PRINT_ERROR(
          "ERROR :: Pages not pinned for file being flushed. Exception "
          "should not have been thrown.");
    }
  }

  // Evict current pages
  {
    // Create new file
    try {
      File::remove("placeHolder.txt");
    } catch (...) {
    }
    File file = File::create("placeHolder.txt");
    Page page;
    Page *ptr = &page;

    // Replace pages
    for (i = 0; i < num; i++) {
      bufMgr->allocPage(file, pid[i], ptr);
    }
    for (i = 0; i < num; i++) {
      bufMgr->unPinPage(file, pid[i], true);
    }
    bufMgr->flushFile(file);
  }

  // Remove old file
  File::remove("placeHolder.txt");

  std::cout << "Test 14 passed\n";
}

void test15(File &file2) {
  // Flush file that has never been added (do nothing)
  bufMgr->flushFile(file2);

  std::cout << "Test 15 passed\n";
}
