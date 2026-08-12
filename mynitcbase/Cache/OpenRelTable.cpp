#include "OpenRelTable.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

OpenRelTable::OpenRelTable() {

    // initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }

    /************ Setting up Relation Cache entries ************/
    
    RecBuffer relCatBlock(RELCAT_BLOCK); // create object for relation catalog block
    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog
    struct RelCacheEntry relCacheEntry; // temporary variable to hold the relation cache entry

    // setting up RELATION CATALOG in Relation Cache (Index 0)
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT); // read the record for Relation Catalog (Slot 0)
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry); // Convert this new record to the struct
    relCacheEntry.recId.block = RELCAT_BLOCK; // set block number
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT; // set slot number

    RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry)); // allocate memory for relation cache entry
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry; // copy the temporary variable to the relation cache

    // setting up ATTRIBUTE CATALOG in Relation Cache (Index 1)
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT); // read the record for Attribute Catalog (Slot 1)
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry); // Convert this new record to the struct
    relCacheEntry.recId.block = RELCAT_BLOCK; // set block number
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT; // set slot number

    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry)); // allocate memory for relation cache entry
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry; // copy the temporary variable to the relation cache

    
    
    /************ Setting up Attribute cache entries ************/
    RecBuffer attrCatBlock(ATTRCAT_BLOCK); // create object for attribute catalog block
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS]; // will store the record from the attribute catalog
    AttrCacheEntry *curr = nullptr, *head = nullptr; // pointers for linked list of attribute cache entries

    // setting up Attributes for RELATION CATALOG (Slots 0-5)
    int numberOfAttributes = RelCacheTable::relCache[RELCAT_RELID]->relCatEntry.numAttrs; // should be 6
    
    head = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    curr = head;

    for(int i = 0; i < numberOfAttributes; i++) {
        attrCatBlock.getRecord(attrCatRecord, i); // read the ith record from attribute catalog
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &curr->attrCatEntry); // Convert this new record to the struct
        
        curr->recId.block = ATTRCAT_BLOCK; // set block number
        curr->recId.slot = i; // set slot number
        
        // Only create next node if we are NOT at the last attribute
        if (i < numberOfAttributes - 1) {
            curr->next = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
            curr = curr->next;
        } else {
            curr->next = nullptr; // set next pointer to nullptr for the last attribute
        }
    }
    AttrCacheTable::attrCache[RELCAT_RELID] = head; // set the head of the linked list in the attribute cache


    // setting up Attributes for ATTRIBUTE CATALOG (Slots 6-11)
    numberOfAttributes = RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry.numAttrs;
    
    head = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    curr = head;

    for(int i = 6; i < 12; i++) {
        attrCatBlock.getRecord(attrCatRecord, i); // read the ith record from attribute catalog
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &curr->attrCatEntry); // Convert this new record to the struct
        
        curr->recId.block = ATTRCAT_BLOCK; // set block number
        curr->recId.slot = i; // set slot number
        
        if (i < 11) { // Only create next node if we are NOT at the last attribute
             curr->next = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
             curr = curr->next;
        } else {
             curr->next = nullptr; // set next pointer to nullptr for the last attribute
        }
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head; // set the head of the linked list in the attribute cache
     
    // setting up STUDENTS relation in Relation Cache if it already exists on disk
    int studentsRelId = loadRelation((char *)"Students");
    if (studentsRelId >= 0) {
        RecId resetIndex = {-1, -1};
        RelCacheTable::setSearchIndex(studentsRelId, &resetIndex);
    }
}

OpenRelTable::~OpenRelTable() {
    // free all the memory that you allocated in the constructor
}

/*
   Loads a relation into the Cache and returns its RelID.
   Returns E_RELNOTEXIST if not found, or E_MAXRELATIONS if cache is full.
*/
int OpenRelTable::loadRelation(char *relName) {
    
    // find a Free Cache Slot (Start from 2, as 0 & 1 are reserved)
    int relId = -1;
    for (int i = 2; i < MAX_OPEN; i++) {
        if (RelCacheTable::relCache[i] == nullptr) {
            relId = i;
            break;
        }
    }
    if (relId == -1) return E_MAXRELATIONS; // Cache is full

    // search Relation Catalog for the relation name
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    struct RelCacheEntry relCacheEntry;
    bool found = false;

    // scan the Relation Catalog Block (Block 4)
    HeadInfo relHeader;
    relCatBlock.getHeader(&relHeader);

    for (int i = 0; i < relHeader.numEntries; i++) {
        relCatBlock.getRecord(relCatRecord, i);
        
        if (strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, relName) == 0) {
            // found the relation
            RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
            relCacheEntry.recId.block = RELCAT_BLOCK;
            relCacheEntry.recId.slot = i;

            // allocate memory and cache it
            RelCacheTable::relCache[relId] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
            *(RelCacheTable::relCache[relId]) = relCacheEntry;
            found = true;
            break;
        }
    }

    if (!found) return E_RELNOTEXIST;


    // search Attribute Catalog for columns (Handles Multiple Blocks)
    int currentBlock = ATTRCAT_BLOCK;
    AttrCacheEntry *head = nullptr;
    AttrCacheEntry *curr = nullptr;

    while (currentBlock != -1) {
        RecBuffer attrCatBlock(currentBlock);
        HeadInfo attrHeader;
        attrCatBlock.getHeader(&attrHeader);

        for (int j = 0; j < attrHeader.numEntries; j++) {
            Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
            attrCatBlock.getRecord(attrCatRecord, j);

            // check if attribute belongs to the requested relation
            if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName) == 0) {
                
                // create linked list node
                AttrCacheEntry* newNode = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
                AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &newNode->attrCatEntry);
                newNode->recId.block = currentBlock;
                newNode->recId.slot = j;
                newNode->next = nullptr;

                // Append to List
                if (head == nullptr) {
                    head = newNode;
                    curr = head;
                } else {
                    curr->next = newNode;
                    curr = curr->next;
                }
            }
        }
        currentBlock = attrHeader.rblock; // move to next block
    }

    AttrCacheTable::attrCache[relId] = head;

    return relId; // return the index where we loaded it
}

/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

    // if relname is RELCAT_RELNAME, return RELCAT_RELID
    if(strcmp(relName, RELCAT_RELNAME) == 0) return RELCAT_RELID;
    // if relname is ATTRCAT_RELNAME, return ATTRCAT_RELID
    if(strcmp(relName, ATTRCAT_RELNAME) == 0) return ATTRCAT_RELID;

    // search all other open relations for the requested name
    for (int relId = 2; relId < MAX_OPEN; relId++) {
        if (RelCacheTable::relCache[relId] != nullptr &&
            strcmp(RelCacheTable::relCache[relId]->relCatEntry.relName, relName) == 0) {
            return relId;
        }
    }

    return E_RELNOTOPEN;
}