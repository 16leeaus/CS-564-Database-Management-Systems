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
 * File:       btree.cpp
 *
 *******************************************************
 *
 * Implementation of a B-Tree data structure
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

#include "btree.h"
#include "exceptions/bad_index_info_exception.h"
#include "exceptions/bad_opcodes_exception.h"
#include "exceptions/bad_scanrange_exception.h"
#include "exceptions/end_of_file_exception.h"
#include "exceptions/file_not_found_exception.h"
#include "exceptions/index_scan_completed_exception.h"
#include "exceptions/no_such_key_found_exception.h"
#include "exceptions/scan_not_initialized_exception.h"
#include "filescan.h"

//#define DEBUG

namespace badgerdb {

/*
 * BTreeIndex Constructor.
 * Check to see if the corresponding index file exists. If so, open the file.
 * If not, create it and insert entries for every tuple in the base relation
 * using FileScan class.
 *
 * @param relationName  	Name of file.
 * @param outIndexName 		Return the name of index file.
 * @param bufMgrIn		Buffer Manager Instance
 * @param attrByteOffset	Offset of attribute, over which index is to be
 * 				built, in the record
 * @param attrType		Datatype of attribute over which index is built
 *
 * @throws  BadIndexInfoException     	If the index file already exists for the
 * 					corresponding attribute, but values in
 * 					metapage(relationName, attribute byte
 * 					offset, attribute type etc.) do not
 * 					match with values received through
 *					constructor parameters.
 */
BTreeIndex::BTreeIndex(const std::string &relationName,
                       std::string &outIndexName, BufMgr *bufMgrIn,
                       const int attrByteOffset, const Datatype attrType) {
  // B+ tree index initialization
  bufMgr = bufMgrIn;
  scanExecuting = false;
  leafOccupancy = INTARRAYLEAFSIZE;
  nodeOccupancy = INTARRAYNONLEAFSIZE;
  attributeType = attrType;
  this->attrByteOffset = attrByteOffset;
  // create index name from relation
  std::ostringstream idxStr;
  idxStr << relationName << '.' << attrByteOffset;
  outIndexName = idxStr.str();
  try {
    // try opening index file
    file = new BlobFile(outIndexName, false);
    headerPageNum = file->getFirstPageNo();
    Page *headerPage;
    bufMgr->readPage(file, headerPageNum, headerPage);
    IndexMetaInfo *metadata = (IndexMetaInfo *)headerPage;
    rootPageNum = metadata->rootPageNo;

    // check that index information is correct
    if (metadata->attrByteOffset != attrByteOffset ||
        metadata->relationName != relationName ||
        metadata->attrType != attributeType) {
      throw BadIndexInfoException("Index information for " + outIndexName +
                                  " is not correct");
    }
    // no need to write page to disk
    bufMgr->unPinPage(file, headerPageNum, false);
  }

  // index file not found, create new one
  catch (FileNotFoundException &e) {
    file = new BlobFile(outIndexName, true);

    // allocate root and header pages
    Page *rootPage;
    Page *headerPage;
    bufMgr->allocPage(file, headerPageNum, headerPage);
    bufMgr->allocPage(file, rootPageNum, rootPage);
    // initialize root information
    ((LeafNodeInt *)(rootPage))->rightSibPageNo = 0;
    ((LeafNodeInt *)(rootPage))->numEntries = 0;

    // initialize metadata information
    IndexMetaInfo *metadata = (IndexMetaInfo *)headerPage;
    metadata->rootPageNo = rootPageNum;
    initRootPageNum = rootPageNum;
    metadata->attrByteOffset = attrByteOffset;
    metadata->attrType = attrType;
    strncpy(metadata->relationName, relationName.c_str(), 20);
    metadata->relationName[19] =
        '\0';  // null terminating character manually added
    // writes pages to disk
    bufMgr->unPinPage(file, headerPageNum, true);
    bufMgr->unPinPage(file, rootPageNum, true);

    // inserting entries for every tuple in the base relation using FileScan
    // class
    try {
      RecordId rid;
      FileScan fileScan(relationName, bufMgr);
      while (1)  // will eventually throw exception when end of file is reached
      {
        fileScan.scanNext(rid);
        insertEntry(fileScan.getRecord().c_str() + attrByteOffset, rid);
      }
    } catch (EndOfFileException &e) {
      bufMgr->flushFile(file);
    }
  }
}

/*
 * BTreeIndex Destructor.
 * End any initialized scan, flush index file, after unpinning any pinned pages,
 * from the buffer manager and delete file instance thereby closing the index
 * file.
 * Destructor should not throw any exceptions. All exceptions should be caught
 * in here itself.
 */
BTreeIndex::~BTreeIndex() {
  // Attempt to end scan, if initialized
  try {
    endScan();
  } catch (ScanNotInitializedException &e) {
  }

  // Deallocate file
  bufMgr->flushFile(file);
  delete file;
}

/*
 * Begin a filtered scan of the index.  For instance, if the method is called
 * using ("a",GT,"d",LTE) then we should seek all entries with a value
 * greater than "a" and less than or equal to "d".
 * If another scan is already executing, that needs to be ended here.
 * Set up all the variables for scan. Start from root to find out the leaf page
 * that contains the first RecordID that satisfies the scan parameters. Keep
 * that page pinned in the buffer pool.
 *
 * @param lowVal	Low value of range, pointer to integer / double / char
 * 			string
 * @param lowOp		Low operator (GT/GTE)
 * @param highVal	High value of range, pointer to integer / double / char
 * 			string
 * @param highOp	High operator (LT/LTE)
 *
 * @throws  BadOpcodesException If lowOp and highOp do not contain one of their
 * 		their expected values
 * @throws  BadScanrangeException If lowVal > highval
 * @throws  NoSuchKeyFoundException If there is no key in the B+ tree that
 * 		satisfies the scan criteria.
 */
void BTreeIndex::startScan(const void *lowValParm, const Operator lowOpParm,
                           const void *highValParm, const Operator highOpParm) {
  // Check that opcodes are valid
  if ((lowOpParm != GT && lowOpParm != GTE) ||
      (highOpParm != LT && highOpParm != LTE)) {
    throw BadOpcodesException();
  }

  // Check that scan range is valid
  if (*((int *)(lowValParm)) > *((int *)(highValParm))) {
    throw BadScanrangeException();
  }

  // Clean up previous execution
  if (scanExecuting) {
    endScan();
  }
  scanExecuting = true;
  first = true;

  // Set operators
  lowOp = lowOpParm;
  highOp = highOpParm;
  lowValInt = *((int *)(lowValParm));
  highValInt = *((int *)(highValParm));
  bool low;
  bool high;

  // Begin Scan at Root
  currentPageNum = rootPageNum;
  bufMgr->readPage(file, currentPageNum, currentPageData);

  // If we are not in a leaf, find leftmost leaf in range
  if (currentPageNum != initRootPageNum) {
    // Travel down levels of B-Tree, check nodes
    // Note: if height is 0, then we skip this step
    for (int i = ((NonLeafNodeInt *)(currentPageData))->level; i > 0; i--) {
      bufMgr->unPinPage(file, currentPageNum, false);

      // Find rightmost child
      currentPageNum = ((NonLeafNodeInt *)(currentPageData))->pageNoArray[0];
      for (int j = 0; j < ((NonLeafNodeInt *)(currentPageData))->numEntries;
           j++) {
        checkRange((void *)(&((LeafNodeInt *)(currentPageData))->keyArray[j]),
                   low, high);
        if (low) {
          currentPageNum =
              ((NonLeafNodeInt *)(currentPageData))->pageNoArray[j];
        }
      }
      bufMgr->readPage(file, currentPageNum, currentPageData);
    }
  }
  // At this point, the current page is a leaf. We must now find the smallest
  // key within the range.
  while (currentPageNum != 0) {
    for (int i = 0; i < ((LeafNodeInt *)(currentPageData))->numEntries; i++) {
      checkRange((void *)(&((LeafNodeInt *)(currentPageData))->keyArray[i]),
                 low, high);

      // If we are within the range, then we found the first matching key!
      if (!low && !high) {
        nextEntry = i;
        return;
      } else if (high) {
        bufMgr->unPinPage(file, currentPageNum, false);
        throw NoSuchKeyFoundException();
      }
    }

    // If we reached here, no key must exist in this page. We read the next page
    nextEntry = 0;
    PageId temp = ((LeafNodeInt *)(currentPageData))->rightSibPageNo;
    bufMgr->unPinPage(file, currentPageNum, false);
    currentPageNum = temp;
    try {
      bufMgr->readPage(file, currentPageNum, currentPageData);
    } catch (...) {
      // If an exception has been thrown, then we will be unable to find a
      // key within the given range, so we throw an exception
      throw NoSuchKeyFoundException();
    }
  }

  // If we reach here, we parsed the entire tree and found no matching key
  throw NoSuchKeyFoundException();
  return;
}

/*
 * Fetch the record id of the next index entry that matches the scan.
 * Return the next record from current page being scanned. If current page has
 * been scanned to its entirety, move on to the right sibling of current page,
 * if any exists, to start scanning that page. Make sure to unpin any pages that
 * are no longer required
 *
 * @param outRid        RecordId of next record found that satisfies the scan
 *                      criteria returned in this
 *
 * @throws ScanNotInitializedException If no scan has been initialized.
 * @throws IndexScanCompletedException If no more records, satisfying the scan
 *                      criteria, are left to be scanned.
 */
void BTreeIndex::scanNext(RecordId &outRid) {
  // Check that scan is executing on valid page
  if (!scanExecuting) {
    // If scan terminated throw exception
    throw ScanNotInitializedException();
  } else if (nextEntry == -1) {
    // If nextEntry is set to -1, then we already reached the end
    bufMgr->unPinPage(file, currentPageNum, false);
    currentPageNum = 0;
    throw IndexScanCompletedException();
  } else if (nextEntry == 0 && !(first)) {
    // If nextEntry is set to 0, there is possibly a valid page to the right
    try {
      PageId temp = ((LeafNodeInt *)(currentPageData))->rightSibPageNo;
      bufMgr->unPinPage(file, currentPageNum, false);
      currentPageNum = temp;
      bufMgr->readPage(file, currentPageNum, currentPageData);
    } catch (...) {
      throw IndexScanCompletedException();
    }
  }

  // Continue scan at current
  first = false;
  bool high;
  bool low;
  checkRange((void *)(&((LeafNodeInt *)(currentPageData))->keyArray[nextEntry]),
             low, high);

  // If the key is within range, extract return data and continue scan
  if (!high) {
    // Unpin the page if it was already pinned, so that the pin count is 1
    try {
      bufMgr->unPinPage(file, currentPageNum, false);
    } catch (...) {
    }
    outRid = ((LeafNodeInt *)(currentPageData))->ridArray[nextEntry];
    bufMgr->readPage(file, currentPageNum, currentPageData);
  } else if (nextEntry != 0) {
    // If the key is not within range, and we are not at a new page, unpin and
    // throw exception as we have found the end of the scanning range
    bufMgr->unPinPage(file, currentPageNum, false);
    throw IndexScanCompletedException();
  } else {
    // If we are at the end of both this node and the range, just throw
    // exception to keep page pinned
    throw IndexScanCompletedException();
  }

  // Set indexing to next available key if we are returning
  if (nextEntry + 1 < ((LeafNodeInt *)(currentPageData))->numEntries) {
    nextEntry = nextEntry + 1;
  } else if (((LeafNodeInt *)(currentPageData))->rightSibPageNo != 0) {
    nextEntry = 0;
  } else {
    nextEntry = -1;
  }

  // Note: we are returning a value in outRid
  return;
}

/*
 * Terminate the current scan. Unpin any pinned pages. Reset scan specific
 * variables.
 *
 * @throws ScanNotInitializedException If no scan has been initialized.
 */
void BTreeIndex::endScan() {
  // Check that scan is executing, update
  if (!scanExecuting) {
    throw ScanNotInitializedException();
  }
  scanExecuting = false;

  // Unpin the current page if it is pinned
  try {
    bufMgr->unPinPage(file, currentPageNum, false);
  } catch (...) {
  }

  // Discard current data from scan
  currentPageData = nullptr;
  currentPageNum = static_cast<PageId>(0);
  nextEntry = -1;
}

/*
 * Private helper method for scanning methods specified where the key lies
 *
 * @param key: the key to be examined
 * @param low: return true if key is below lowVal
 * @param high: return true if key is above highVal
 */
const void BTreeIndex::checkRange(const void *key, bool &low, bool &high) {
  // Default
  high = false;
  low = false;

  // Check high
  if (*((int *)(key)) > highValInt) {
    high = true;
  } else if (*((int *)(key)) == highValInt && int(highOp) == 0) {
    high = true;
  }

  // Check low
  if (*((int *)(key)) < lowValInt) {
    low = true;
  } else if (*((int *)(key)) == lowValInt && int(lowOp) == 3) {
    low = true;
  }

  return;
}

/*
 * Insert a new entry using the pair <value,rid>.
 * Start from root to recursively find out the leaf to insert the entry in.
 * The insertion may cause splitting of leaf node. This splitting will require
 * addition of new leaf page number entry into the parent non-leaf, which may
 * in-turn get split.
 * This may continue all the way upto the root causing the root to get split.
 * If root gets split, metapage needs to be changed accordingly.
 * Pages must be unpinned as soon as possible.
 *
 * @param key 	Key to insert, pointer to integer/double/char string
 * @param rid	Record ID of a record whose entry is getting inserted into the
 * 		index.
 */
void BTreeIndex::insertEntry(const void *key, const RecordId rid) {
  // Read root
  Page *rootPage;
  bufMgr->readPage(file, rootPageNum, rootPage);
  bool rootIsLeaf = rootPageNum == initRootPageNum;

  PageKeyPair<int> pivot;
  PageKeyPair<int> *pivotPtr = &pivot;

  // Insert by recursing
  recInsertEntry(rootPage, rootPageNum, key, rid, rootIsLeaf, pivotPtr);
}

/*
 * Recursive helper function for insertion
 *
 * @param currPage: page associated with insertion
 * @param currId: PageId associated with the given page (inserted for
 * internal)
 * @param key: pointer to key used for insertion
 * @param rid: record id used for insertion (leaf)
 * @param leaf: indicates if we are inserting into a leaf
 * @param pivot: pivot key/page pair used if we are splitting at current level
 */
const void BTreeIndex::recInsertEntry(Page *currPage, PageId currId,
                                      const void *key, const RecordId rid,
                                      bool leaf, PageKeyPair<int> *&pivot) {
  // Insertion into leaf
  if (leaf) {
    // Check if the current leaf is not full
    if (((LeafNodeInt *)(currPage))->numEntries < leafOccupancy) {
      // If not full, insert without splitting
      addEntry(currPage, currId, key, rid, true);
      bufMgr->unPinPage(file, currId, true);
      pivot = nullptr;
      return;
    } else {
      // If full, insert and split
      addEntryAndSplit(currPage, currId, key, rid, true, pivot);
      bufMgr->unPinPage(file, currId, true);
      return;
    }
  }

  // Insertion into internal node
  else {
    // Get next PageId for recursive call
    PageId nextId;
    for (int i = 0; i < ((NonLeafNodeInt *)(currPage))->numEntries; i++) {
      // Internal insertion
      if (*((int *)(key)) < ((NonLeafNodeInt *)(currPage))->keyArray[i]) {
        nextId = ((NonLeafNodeInt *)(currPage))->pageNoArray[i];
        break;
      }

      // End insertion
      else if (i + 1 == ((NonLeafNodeInt *)(currPage))->numEntries) {
        nextId = ((NonLeafNodeInt *)(currPage))->pageNoArray[i + 1];
      }
    }

    // Get next Page for recursive call
    Page *nextPage;
    bufMgr->readPage(file, nextId, nextPage);

    // Make recursive call
    bool enteringLeaf = ((NonLeafNodeInt *)(currPage))->level == 1;
    recInsertEntry(nextPage, nextId, key, rid, enteringLeaf, pivot);

    // If we split in the recursive call but are not full here, do not split
    if (pivot != nullptr &&
        ((NonLeafNodeInt *)(currPage))->numEntries < nodeOccupancy) {
      addEntry(currPage, pivot->pageNo, (void *)(&(pivot->key)), rid, false);
      pivot = nullptr;
      bufMgr->unPinPage(file, currId, true);
      return;
    }
    // If we split in the recursive call and are full here, split here as well
    else if (pivot != nullptr) {
      addEntryAndSplit(currPage, pivot->pageNo, (void *)(&(pivot->key)), rid,
                       false, pivot);
      bufMgr->unPinPage(file, currId, true);
      return;
    }
    // If we did not split, return
    else {
      bufMgr->unPinPage(file, currId, false);
      pivot = nullptr;
      return;
    }
  }
}

/*
 * Add into node, shifting for internal insertion if necessary
 *
 * @param currPage: page associated with insertion
 * @param currId: PageId associated with the given page (inserted for
 * internal)
 * @param key: pointer to key used for insertion
 * @param rid: record id used for insertion (leaf)
 * @param leaf: indicates if we are inserting into a leaf
 */
const void BTreeIndex::addEntry(Page *currPage, PageId currId, const void *key,
                                const RecordId rid, bool leaf) {
  // Insert into leaf
  if (leaf) {
    // If the leaf is empty, use index 0, increment entries and return
    if (((LeafNodeInt *)(currPage))->numEntries == 0) {
      ((LeafNodeInt *)(currPage))->keyArray[0] = *((int *)(key));
      ((LeafNodeInt *)(currPage))->ridArray[0] = rid;
      ((LeafNodeInt *)(currPage))->numEntries++;
      return;
    }

    // If the leaf is nonempy, find insertion index
    int insertIndex = ((LeafNodeInt *)(currPage))->numEntries;  // Default: end
    // Parse to see if we should insert before the end
    for (int i = 0; i < ((LeafNodeInt *)(currPage))->numEntries; i++) {
      if (*((int *)(key)) < ((LeafNodeInt *)(currPage))->keyArray[i]) {
        insertIndex = i;
        break;
      }
    }

    // Shift if necessary
    for (int i = ((LeafNodeInt *)(currPage))->numEntries; i > insertIndex;
         i--) {
      ((LeafNodeInt *)(currPage))->keyArray[i] =
          ((LeafNodeInt *)(currPage))->keyArray[i - 1];
      ((LeafNodeInt *)(currPage))->ridArray[i] =
          ((LeafNodeInt *)(currPage))->ridArray[i - 1];
    }
    // Place new entry into index
    ((LeafNodeInt *)(currPage))->keyArray[insertIndex] = *((int *)(key));
    ((LeafNodeInt *)(currPage))->ridArray[insertIndex] = rid;

    // Increment entries and return
    ((LeafNodeInt *)(currPage))->numEntries++;
    return;

    // Insert into Non-Leaf Node
  } else {
    // If the node is nonempy, find insertion index
    int insertIndex = 0;  // Default: start
    // Parse to see where we should insert
    for (int i = 0; i < ((NonLeafNodeInt *)(currPage))->numEntries; i++) {
      if (*((int *)(key)) < ((NonLeafNodeInt *)(currPage))->keyArray[i]) {
        insertIndex = i;
        break;
      } else if (i + 1 == ((NonLeafNodeInt *)(currPage))->numEntries) {
        insertIndex = i + 1;
      }
    }
    // Shift if necessary
    for (int i = ((NonLeafNodeInt *)(currPage))->numEntries; i > insertIndex;
         i--) {
      ((NonLeafNodeInt *)(currPage))->keyArray[i] =
          ((NonLeafNodeInt *)(currPage))->keyArray[i - 1];
      ((NonLeafNodeInt *)(currPage))->pageNoArray[i + 1] =
          ((NonLeafNodeInt *)(currPage))->pageNoArray[i];
    }

    // Place new entry into index
    ((NonLeafNodeInt *)(currPage))->keyArray[insertIndex] = *((int *)(key));
    ((NonLeafNodeInt *)(currPage))->pageNoArray[insertIndex + 1] = currId;

    // Increment entries and return
    ((NonLeafNodeInt *)(currPage))->numEntries++;
    return;
  }
}

/*
 * Split current node and then insert current data
 *
 * @param currPage: page associated with insertion
 * @param currId: PageId associated with the given page (inserted for
 * internal)
 * @param key: pointer to key used for insertion
 * @param rid: record id used for insertion (leaf)
 * @param leaf: indicates if we are inserting into a leaf
 * @param pivot: pivot key/page pair used if we are splitting at current level
 */
const void BTreeIndex::addEntryAndSplit(Page *currPage, PageId currId,
                                        const void *key, const RecordId rid,
                                        bool leaf, PageKeyPair<int> *&pivot) {
  // Fields used for insertion into new page
  int pivotKey;
  PageId pivotId;
  Page *nextPage;
  PageId nextId;
  int splitIndex;

  // Leaf: split, insert
  if (leaf) {
    // Create new leaf page for rightmost entries
    bufMgr->allocPage(file, nextId, nextPage);
    ((LeafNodeInt *)(nextPage))->numEntries = 0;

    // Insert into new node, decrement entries
    splitIndex = (leafOccupancy + 1) / 2;
    for (int i = 0; i < splitIndex; i++) {
      ((LeafNodeInt *)(nextPage))->keyArray[i] =
          ((LeafNodeInt *)(currPage))->keyArray[i + splitIndex];
      ((LeafNodeInt *)(nextPage))->ridArray[i] =
          ((LeafNodeInt *)(currPage))->ridArray[i + splitIndex];
      ((LeafNodeInt *)(currPage))->numEntries--;
      ((LeafNodeInt *)(nextPage))->numEntries++;
    }

    // Insert current entry into node based on key comparison
    if (*((int *)(key)) >
        ((LeafNodeInt *)(currPage))->keyArray[splitIndex - 1]) {
      addEntry(nextPage, nextId, key, rid, true);
    } else {
      addEntry(currPage, currId, key, rid, true);
    }

    // Update references
    ((LeafNodeInt *)(nextPage))->rightSibPageNo =
        ((LeafNodeInt *)(currPage))->rightSibPageNo;
    ((LeafNodeInt *)(currPage))->rightSibPageNo = nextId;
    pivotKey = ((LeafNodeInt *)(nextPage))->keyArray[0];
    pivotId = nextId;
    bufMgr->unPinPage(file, nextId, true);

    // Internal node: split, update, add
  } else {
    // Create new node page for rightmost entries
    bufMgr->allocPage(file, nextId, nextPage);
    ((NonLeafNodeInt *)(nextPage))->numEntries = 0;
    ((NonLeafNodeInt *)(nextPage))->level =
        ((NonLeafNodeInt *)(currPage))->level;

    // Insert into new node, decrement entries
    splitIndex = (nodeOccupancy + 1) / 2;
    for (int i = 0; i < nodeOccupancy - splitIndex; i++) {
      ((NonLeafNodeInt *)(nextPage))->keyArray[i] =
          ((NonLeafNodeInt *)(currPage))->keyArray[i + splitIndex];
      ((NonLeafNodeInt *)(nextPage))->pageNoArray[i] =
          ((NonLeafNodeInt *)(currPage))->pageNoArray[i + splitIndex];
      ((NonLeafNodeInt *)(nextPage))->numEntries++;
      ((NonLeafNodeInt *)(currPage))->numEntries--;
    }

    // Add last page number
    ((NonLeafNodeInt *)(nextPage))->pageNoArray[nodeOccupancy - splitIndex] =
        ((NonLeafNodeInt *)(currPage))->pageNoArray[nodeOccupancy];

    // Insert middle entry into node based on key comparison
    if ((pivot->key) >
        ((NonLeafNodeInt *)(currPage))->keyArray[splitIndex - 1]) {
      addEntry(nextPage, pivot->pageNo, ((void *)(&(pivot->key))), rid, false);
    } else {
      addEntry(currPage, pivot->pageNo, ((void *)(&(pivot->key))), rid, false);
    }

    pivotKey = (((NonLeafNodeInt *)(currPage))->keyArray[splitIndex]);
    pivotId = ((NonLeafNodeInt *)(nextPage))->pageNoArray[0];
    bufMgr->unPinPage(file, nextId, true);
    bufMgr->readPage(file, currId, currPage);
  }

  // If currId is rootId, then we must modify the root
  if (currId == rootPageNum) {
    // Get level of new root
    int level;
    // If we are in a leaf, set to 1
    if (currId == initRootPageNum) {
      level = 1;
      // If we are not in a leaf, read root to extract level, increment
    } else {
      Page *rootPage;
      bufMgr->readPage(file, rootPageNum, rootPage);
      level = ((NonLeafNodeInt *)(rootPage))->level + 1;
      bufMgr->unPinPage(file, rootPageNum, false);
    }

    // Allocate pages for new root and corresponding metadata
    // unpin pages immediately after use
    Page *nextRoot;
    Page *nextMetaData;
    PageId nextRootId;
    bufMgr->allocPage(file, nextRootId, nextRoot);
    // Set new root fields
    ((NonLeafNodeInt *)(nextRoot))->level = level;
    ((NonLeafNodeInt *)(nextRoot))->keyArray[0] = pivotKey;
    ((NonLeafNodeInt *)(nextRoot))->pageNoArray[0] = currId;
    ((NonLeafNodeInt *)(nextRoot))->pageNoArray[1] = nextId;
    ((NonLeafNodeInt *)(nextRoot))->numEntries = 1;
    bufMgr->unPinPage(file, nextRootId, true);
    // Reset root and its corresponding metadata
    bufMgr->readPage(file, headerPageNum, nextMetaData);
    ((IndexMetaInfo *)(nextMetaData))->rootPageNo = nextRootId;
    bufMgr->unPinPage(file, headerPageNum, true);
    rootPageNum = nextRootId;
  }

  // Set up return value for pivot
  if (!leaf) {
    bufMgr->unPinPage(file, pivot->pageNo, true);
  }
  pivot->set(pivotId, pivotKey);
  return;
}
}  // namespace badgerdb
