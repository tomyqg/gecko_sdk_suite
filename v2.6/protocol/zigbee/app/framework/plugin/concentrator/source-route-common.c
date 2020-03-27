/***************************************************************************//**
 * @file
 * @brief Common code used for managing source routes on both node-based
 * and host-based gateways. See source-route.c for node-based gateways and
 * source-route-host.c for host-based gateways.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include PLATFORM_HEADER
#include "stack/include/ember-types.h"
#include "source-route-common.h"

// The number of entries in use.
#ifndef EMBER_TEST
static
#endif // EMBER_TEST
uint8_t entryCount = 0;

// The index of the most recently added entry.
#ifndef EMBER_TEST
static
#endif // EMBER_TEST
uint8_t newestIndex = NULL_INDEX;

#ifdef EMBER_TEST
 #define testAssert assert
#else // EMBER_TEST
 #define testAssert(x) do {} while (0)
#endif // EMBER_TEST

// -----------------------------------------------------------------------------
// Prototypes
uint8_t getSourceRouteIndexToDelete(void);

// -----------------------------------------------------------------------------
// Functions

// Return the index of the entry with the specified destination.
uint8_t sourceRouteFindIndex(EmberNodeId id)
{
  if (sourceRouteTableSize == 0) {
    return NULL_INDEX;
  }
  uint8_t i;
  for (i = 0; i < entryCount; i++) {
    if (sourceRouteTable[i].destination == id) {
      return i;
    }
  }
  return NULL_INDEX;
}

bool sourceRouteIndexInUseByFurtherNode(uint8_t index)
{
  uint8_t i;
  if (index == NULL_INDEX) {
    return false;
  }
  for (i = 0; i < entryCount; i++) {
    if (sourceRouteTable[i].closerIndex == index) {
      return true;
    }
  }
  return false;
}

void sourceRouteClearTable(void)
{
  entryCount = 0;
  newestIndex = NULL_INDEX;
  MEMSET(sourceRouteTable,
         0,
         sizeof(SourceRouteTableEntry) * sourceRouteTableSize);
}

// Create an entry with the given id or update an existing entry. furtherIndex
// is the entry one hop further from the gateway.

// CAUTION: make sure calling this function updates all the related entries,
// and not just one specific entry. Example correct use case could be seen in:
// emberIncomingRouteRecordHandler(....)
uint8_t sourceRouteAddEntry(EmberNodeId id, uint8_t furtherIndex)
{
  if (sourceRouteTableSize == 0) {
    return NULL_INDEX;
  }
  // See if the id already exists in the table.
  uint8_t index = sourceRouteFindIndex(id);
  uint8_t i;

  if (index == NULL_INDEX) {
    if (entryCount < sourceRouteTableSize) {
      // No existing entry. Table is not full. Add new entry.
      index = entryCount;
      entryCount += 1;
    } else {
      // No existing entry. Table is full. Replace oldest entry.
      index = newestIndex;
      while (sourceRouteTable[index].olderIndex != NULL_INDEX) {
        index = sourceRouteTable[index].olderIndex;
      }
    }
  }

  // Update the pointers (only) if something has changed.
  if (index != newestIndex) {
    for (i = 0; i < entryCount; i++) {
      if (sourceRouteTable[i].olderIndex == index) {
        sourceRouteTable[i].olderIndex = sourceRouteTable[index].olderIndex;
        break;
      }
    }
    sourceRouteTable[index].olderIndex = newestIndex;
    newestIndex = index;
  }
  // Add the entry.
  sourceRouteTable[index].destination = id;
  sourceRouteTable[index].closerIndex = NULL_INDEX;
  // The current index is one hop closer to the gateway than furtherIndex.
  if (furtherIndex != NULL_INDEX) {
    sourceRouteTable[furtherIndex].closerIndex = index;
  }

  // Return the current index to save the caller having to look it up.
  return index;
}

// Returns index with destination set to EMBER_NULL_NODE_ID, meaning the entry
// should be deleted, else entryCount
uint8_t getSourceRouteIndexToDelete(void)
{
  uint8_t i;
  for (i = 0; i < entryCount; i++) {
    if (sourceRouteTable[i].destination == EMBER_NULL_NODE_ID) {
      break;
    }
  }
  return i;
}

EmberStatus sourceRouteDeleteEntry(EmberNodeId id)
{
  uint8_t index = sourceRouteFindIndex(id);
  uint8_t i;
  uint8_t indexToDelete;

  if (index == NULL_INDEX) {
    return EMBER_NOT_FOUND;
  }

  // Mark it for deletion by setting destination to EMBER_NULL_NODE_ID
  sourceRouteTable[index].destination = EMBER_NULL_NODE_ID;

  indexToDelete = index;

  while (indexToDelete != entryCount) {
    // Update the newestIndex and olderIndex mappings if necessary
    if (newestIndex == indexToDelete) {
      // The newest entry is the one to delete. Update newestIndex to the
      // previous newest one
      newestIndex = sourceRouteTable[indexToDelete].olderIndex;
    } else {
      // The index to delete is not the newestIndex, so find the entry whose
      // olderIndex points to this index and update it
      for (i = 0; i < entryCount; i++) {
        if (indexToDelete == sourceRouteTable[i].olderIndex) {
          break;
        }
      }

      // We must have found the entry that points to this one
      // If this asserts then the olderIndex links are broken
      testAssert(i != entryCount);

      sourceRouteTable[i].olderIndex = sourceRouteTable[indexToDelete].olderIndex;
    }

    // Shift the memory over by one index
    uint8_t numEntriesToShift = (entryCount - indexToDelete - 1);
    MEMMOVE(&sourceRouteTable[indexToDelete],
            &sourceRouteTable[indexToDelete + 1],
            numEntriesToShift * sizeof(sourceRouteTable[0]));

    // Decrement the total number of entries
    entryCount -= 1;

    // Decrement newestIndex if it was greater than or equal to the index wiped
    if ((newestIndex != NULL_INDEX) && (newestIndex >= indexToDelete)) {
      newestIndex -= 1;
    }

    // Any entry whose closerIndex was equal to currentIndexToDelete must be
    // removed. Else, if the entry's closerIndex was equal to or greater than
    // currentIndexToDelete must be decremented by one, since we shifted memory
    // Any entry whose olderIndex was equal to or greater than
    // currentIndexToDelete must be decremented by one, since we shifted memory
    for (i = 0; i < entryCount; i++) {
      if (sourceRouteTable[i].closerIndex == indexToDelete) {
        sourceRouteTable[i].destination = EMBER_NULL_NODE_ID;
      } else if ((sourceRouteTable[i].closerIndex != NULL_INDEX)
                 && (sourceRouteTable[i].closerIndex >= indexToDelete)) {
        sourceRouteTable[i].closerIndex -= 1;
      }
      if ((sourceRouteTable[i].olderIndex != NULL_INDEX)
          && (sourceRouteTable[i].olderIndex >= indexToDelete)) {
        sourceRouteTable[i].olderIndex -= 1;
      }
    }

    // Move to the next index to delete if there is one
    indexToDelete = getSourceRouteIndexToDelete();
  }

  return EMBER_SUCCESS;
}

uint8_t sourceRouteAddEntryWithCloserNextHop(EmberNodeId newId,
                                             EmberNodeId closerNodeId)
{
  if (sourceRouteTableSize == 0) {
    return NULL_INDEX;
  }
  uint8_t closerIndex = sourceRouteFindIndex(closerNodeId);
  if (closerIndex != NULL_INDEX) {
    if (sourceRouteIndexInUseByFurtherNode(sourceRouteFindIndex(newId))) {
      // If this node isn't a leaf in the source route tree,
      // that means it's a router, used as a source route for someone
      // else.  That someone else could be the new parent, which would
      // create a loop.  If they're all routers, the existing route
      // should still be fine, so we leave it unchanged.
      return NULL_INDEX;
    }
    uint8_t index = sourceRouteAddEntry(newId, NULL_INDEX);
    if (index != NULL_INDEX) {
      // emzigbee-408
      // we refresh all the ancestors of the newId, instead of simply
      // putting the index of the newId's parent as its closerIndex
      uint8_t curFurthurIndex = index;
      uint8_t curCloserIndex = closerIndex;
      uint8_t nextFurthurIndex, nextCloserIndex, ancestorIndex;
      while (curCloserIndex != NULL_INDEX) {
        nextFurthurIndex = curCloserIndex;
        nextCloserIndex = sourceRouteTable[nextFurthurIndex].closerIndex;
        // next call refreshes the closerNodeId entry and the rest of the ancestors
        ancestorIndex = sourceRouteAddEntry(sourceRouteTable[curCloserIndex].destination, curFurthurIndex);
        if (ancestorIndex == index) { // our new entry has been replaced by rewritting one of its ancestors
          index = NULL_INDEX; // In this case we refreshed the entries of the new nodes ancestor
          // without receiving a new route record message and even without inserting the node that
          // triggered this whole refreshment process.
        }
        curFurthurIndex = nextFurthurIndex;
        curCloserIndex = nextCloserIndex;
      }
    }
    return index;
  }
  return NULL_INDEX;
}

void sourceRouteInit(void)
{
  entryCount = 0;
}

uint8_t sourceRouteGetCount(void)
{
  return entryCount;
}

uint8_t emberGetSourceRouteTableFilledSize(void)
{
  return entryCount;
}

uint8_t emberGetSourceRouteTableTotalSize(void)
{
  return sourceRouteTableSize;
}
