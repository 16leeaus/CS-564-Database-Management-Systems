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
 * Student ID: alee88@wisc.edu
 * Wisc ID:    9073532831
 *
 * Class:      Comp Sci 564
 * Semester:   Spring 2022
 * Project:    Assignment 2
 * File:       buffer.cpp
 *******************************************************
 *
 * This file implements the buffer manager class, which
 * processes queries by pinning and unpinning pages
 *
 *******************************************************
 *
 * NOTE:
 *
 * @author See Contributors.txt for code contributors and
 * overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences
 * Department, University of Wisconsin-Madison.
 *
 */ //**************************************************

#include "buffer.h"

#include <iostream>
#include <memory>

#include "exceptions/bad_buffer_exception.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/hash_not_found_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"

namespace badgerdb {

constexpr int HASHTABLE_SZ(int bufs) { return ((int)(bufs * 1.2) & -2) + 1; }

//----------------------------------------
// Constructor of the class BufMgr
//----------------------------------------

/**
 * Print member variable values.
 */
BufMgr::BufMgr(std::uint32_t bufs)
    : numBufs(bufs),
      hashTable(HASHTABLE_SZ(bufs)),
      bufDescTable(bufs),
      bufPool(bufs) {
  for (FrameId i = 0; i < bufs; i++) {
    bufDescTable[i].frameNo = i;
    bufDescTable[i].valid = false;
  }
  clockHand = bufs - 1;
}

/**
 * Advance clock to next frame in the buffer pool
 */
void BufMgr::advanceClock() { clockHand = (clockHand + 1) % numBufs; }

/**
 * Allocate a free frame.
 *
 * @param frame Frame reference, frame ID of allocated frame returned
 * via this variable
 * @throws BufferExceededException If no such buffer is found which can be
 * allocated
 */
void BufMgr::allocBuf(FrameId& frame) {
  // Circle clock twice
  for (uint32_t i = 0; i < (numBufs * 2); i += 1) {
    // If this frame is not valid, allocate and return
    if (!bufDescTable[clockHand].valid) {
      frame = clockHand;
      return;
    }

    // If this frame is valid, check to see if we can evict
    else {
      // If referenced, unmark and proceed
      if (bufDescTable[clockHand].refbit) {
        bufDescTable[clockHand].refbit = 0;
      }

      // Not pinned and not dirty: remove and allocate
      else if ((bufDescTable[clockHand].pinCnt == 0) &&
               !(bufDescTable[clockHand].dirty)) {
        // Check that no other pages are pinned
        try {
          hashTable.remove(bufDescTable[clockHand].file,
                           bufDescTable[clockHand].pageNo);
          frame = clockHand;
          return;
        } catch (PagePinnedException& e) {
        }
      }

      // Pinned and dirty: flush, allocate
      // Note that flushFile removes from hash table and
      // clears entry
      else if ((bufDescTable[clockHand].pinCnt == 0) &&
               bufDescTable[clockHand].dirty) {
        // Check that no other pages are pinned
        try {
          flushFile(bufDescTable[clockHand].file);
          frame = clockHand;
          return;
        } catch (PagePinnedException& e) {
        }
      }
    }

    // Advance the clock for next iteration
    advanceClock();
  }

  // If we reach here, no buffer can be allocated, so we throw an exception
  throw BufferExceededException();
}

/**
 * Reads the given page from the file into a frame and returns the pointer to
 * page. If the requested page is already present in the buffer pool pointer
 * to that frame is returned otherwise a new frame is allocated from the
 * buffer pool for reading the page.
 *
 * @param file File object
 * @param PageNo Page number in the file to be read
 * @param page Reference to page pointer. Used to fetch the Page object
 * in which requested page from file is read in.
 */
void BufMgr::readPage(File& file, const PageId pageNo, Page*& page) {
  try {
    // Check to see if file is in the buffer pool
    FrameId frame;
    hashTable.lookup(file, pageNo, frame);

    // Update ref bit and pin count
    bufDescTable[frame].refbit = true;
    bufDescTable[frame].pinCnt += 1;
    bufStats.accesses++;

    // Return
    page = &bufPool[frame];
    return;

  } catch (HashNotFoundException& e) {
    // If no match was found, get frame from buffer pool
    // and read page into frame
    FrameId frame;
    allocBuf(frame);
    bufPool[frame] = file.readPage(pageNo);
    hashTable.insert(file, bufPool[frame].page_number(), frame);
    bufDescTable[frame].Set(file, bufPool[frame].page_number());
    bufStats.diskreads++;

    // return
    page = &bufPool[frame];
    return;
  }
}

/**
 * Unpin a page from memory since it is no longer required for it to remain in
 * memory.
 *
 * @param file File object
 * @param PageNo Page number
 * @param dirty True if the page to be unpinned needs to be
 * marked dirty
 * @throws PageNotPinnedException If the page is not already pinned
 */
void BufMgr::unPinPage(File& file, const PageId pageNo, const bool dirty) {
  try {
    // Confim that the page is currently present, get frame number
    FrameId frame;
    hashTable.lookup(file, pageNo, frame);

    // If the page is not pinned, throw an exception
    if (!bufDescTable[frame].pinCnt) {
      throw PageNotPinnedException(file.filename(), pageNo, frame);
    }

    // If the page is dirty, set the dirty bit
    if (dirty) {
      bufDescTable[frame].dirty = true;
    }

    // Decrement pin count and return
    bufDescTable[frame].pinCnt -= 1;
    return;

  } catch (HashNotFoundException& e) {
    // If the hash was not found, we can simply return
    return;
  }
}

/** Allocates a new, empty page in the file and returns the Page object.
 * The newly allocated page is also assigned a frame in the buffer pool.
 *
 * @param file File object
 * @param PageNo Page number. The number assigned to the page in the file is
 * returned via this reference.
 * @param page Reference to page pointer. The newly allocated in-memory
 * Page object is returned via this reference.
 */
void BufMgr::allocPage(File& file, PageId& pageNo, Page*& page) {
  // Get frame from buffer pool
  FrameId frame;
  allocBuf(frame);

  // Allocate page within buffer pool
  bufPool[frame] = file.allocatePage();
  hashTable.insert(file, bufPool[frame].page_number(), frame);
  bufDescTable[frame].Set(file, bufPool[frame].page_number());

  // Return values
  pageNo = bufPool[frame].page_number();
  page = &bufPool[frame];
  return;
}

/**
 * Writes out all dirty pages of the file to disk.
 * All the frames assigned to the file need to be unpinned from buffer pool
 * before this function can be successfully called. Otherwise Error returned.
 *
 * @param file File object
 * @throws PagePinnedException If any page of the file is pinned in the
 * buffer pool
 * @throws BadBufferException If any frame allocated to the file is found to
 * be invalid
 */
void BufMgr::flushFile(File& file) {
  // Step 1: iterate over buffers and check that all pages belonging
  // to this file are both valid and dirty
  for (uint32_t i = 0; i < numBufs; i += 1) {
    // If this frame belongs to the file, but is not valid,
    // throw BadBufferException
    if (&(bufDescTable[i].file) == &file && !bufDescTable[clockHand].valid) {
      throw BadBufferException(bufDescTable[i].frameNo, bufDescTable[i].dirty,
                               bufDescTable[i].valid, bufDescTable[i].refbit);

      // If this frame belongs to the file, but it is pinned,
      // throw PagePinnedException
    } else if (bufDescTable[i].file == file && bufDescTable[i].pinCnt) {
      throw PagePinnedException(file.filename(), bufDescTable[i].pageNo,
                                bufDescTable[i].frameNo);
    }
  }

  // Step 2: if all pages are valid, write all of the dirty pages to disk
  for (uint32_t i = 0; i < numBufs; i += 1) {
    // If this frame belongs to the file but is both valid and
    // dirty, then write it and clear
    if ((bufDescTable[i].file) == (file) && bufDescTable[i].dirty) {
      file.writePage(bufPool[i]);
      bufStats.diskwrites++;
      bufDescTable[i].dirty = false;
      hashTable.remove(file, bufDescTable[i].pageNo);
      bufDescTable[i].clear();
    }
  }
}
/**
 * Delete page from file and also from buffer pool if present.
 * Since the page is entirely deleted from file, its unnecessary to see if the
 * page is dirty.
 *
 * @param file File object
 * @param PageNo Page number
 */
void BufMgr::disposePage(File& file, const PageId PageNo) {
  try {
    // Confim that the page is currently present, get frame number
    FrameId frame;
    hashTable.lookup(file, PageNo, frame);

    // Remove and return
    bufDescTable[frame].clear();
    hashTable.remove(file, PageNo);

  } catch (HashNotFoundException& e) {
  }

  // Regardless if the hash was found, we must delete the page from the file
  file.deletePage(PageNo);
  return;
}

/**
 * Print member variable values.
 */
void BufMgr::printSelf(void) {
  int validFrames = 0;

  for (FrameId i = 0; i < numBufs; i++) {
    std::cout << "FrameNo:" << i << " ";
    bufDescTable[i].Print();
    if (bufDescTable[i].valid) {
      validFrames++;
    }
  }
  std::cout << "Total Number of Valid Frames:" << validFrames << "\n";
}

}  // namespace badgerdb
