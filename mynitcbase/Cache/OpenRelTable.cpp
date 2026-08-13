#include "OpenRelTable.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>


OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

AttrCacheEntry* OpenRelTable::createAttrCacheLinkedList(int numberOfAttributes) {
    AttrCacheEntry* head = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheEntry* curr = head;

    for(int i = 0; i < numberOfAttributes; i++) {
        // Only create next node if we are NOT at the last attribute
        if (i < numberOfAttributes - 1) {
            curr->next = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
            curr = curr->next;
        } else {
            curr->next = nullptr; // set next pointer to nullptr for the last attribute
        }
    }
    return head;
}

OpenRelTable::OpenRelTable() {

    // initialize relCache and attrCache with nullptr and tableMetaInfo.free with true
    for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
        tableMetaInfo[i].free = true;
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


    /************ Setting up tableMetaInfo entries ************/

    // in the tableMetaInfo array
    // set free = false for RELCAT_RELID and ATTRCAT_RELID
    tableMetaInfo[RELCAT_RELID].free = false;
    tableMetaInfo[ATTRCAT_RELID].free = false;
    // set relname for RELCAT_RELID and ATTRCAT_RELID
    strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
}

OpenRelTable::~OpenRelTable() {

    // close all open relations (from rel-id = 2 onwards. Why?)
    for (int i = 2; i < MAX_OPEN; ++i) {
        if (!tableMetaInfo[i].free) {
        OpenRelTable::closeRel(i); // we will implement this function later
        }
    }
    // free the memory allocated for rel-id 0 and 1 in the caches
    for (int i = 0; i < 2; ++i) {
        // free relation cache entry
        if (RelCacheTable::relCache[i] != nullptr) {
            free(RelCacheTable::relCache[i]);
            RelCacheTable::relCache[i] = nullptr;
        }

        // free attribute cache linked list
        AttrCacheEntry* curr = AttrCacheTable::attrCache[i];
        while (curr != nullptr) {
            AttrCacheEntry* temp = curr;
            curr = curr->next;
            free(temp);
        }
        AttrCacheTable::attrCache[i] = nullptr;
    }
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
    /* traverse through the tableMetaInfo array, find the entry in the Open Relation Table corresponding to relName.*/
    for(int i = 0; i < MAX_OPEN; i++) {
        if(!tableMetaInfo[i].free && strcmp(tableMetaInfo[i].relName, relName) == 0) {
            return i; // if found return the relation id
        }
    }
    // if found return the relation id, else indicate that the relation do not
    // have an entry in the Open Relation Table.
    return E_RELNOTOPEN;
}

int OpenRelTable::getFreeOpenRelTableEntry() {

    /* traverse through the tableMetaInfo array, find a free entry in the Open Relation Table.*/
    for(int i = 0; i < MAX_OPEN; i++) {
        if(tableMetaInfo[i].free) {
            return i; // if found return the relation id
        }
    }

    // if found return the relation id, else return E_CACHEFULL.
    return E_CACHEFULL;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
    
    // (checked using OpenRelTable::getRelId())
    int relId = getRelId(relName);
    if(relId >= 0){
        // return that relation id;
        return relId;
    }

    // let relId be used to store the free slot.
    /* find a free slot in the Open Relation Table using OpenRelTable::getFreeOpenRelTableEntry(). */
    relId = getFreeOpenRelTableEntry();

    if (relId == E_CACHEFULL){ // free slot not available
        return E_CACHEFULL;
    }

    /****** Setting up Relation Cache entry for the relation ******/

    /* search for the entry with relation name, relName, in the Relation Catalog using
        BlockAccess::linearSearch().
        Care should be taken to reset the searchIndex of the relation RELCAT_RELID
        before calling linearSearch().*/

    // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
    Attribute attrVal;
    strcpy(attrVal.sVal, relName);
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    
    RecId relcatRecId;
    relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, attrVal, EQ);

    if (relcatRecId.block == -1 && relcatRecId.slot == -1) {
        // (the relation is not found in the Relation Catalog.)
        return E_RELNOTEXIST;
    }

    /* read the record entry corresponding to relcatRecId and create a relCacheEntry
        on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
        update the recId field of this Relation Cache entry to relcatRecId.
        use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
        NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
    */
    RecBuffer relCatBlock(relcatRecId.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    RelCacheEntry *relCacheBuffer = nullptr;

    relCatBlock.getRecord(relCatRecord, relcatRecId.slot); // read the record

    relCacheBuffer = (RelCacheEntry*) malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheBuffer->relCatEntry);

    // update the recId field of this Relation Cache entry to relcatRecId.
    relCacheBuffer->recId.block = relcatRecId.block;
    relCacheBuffer->recId.slot = relcatRecId.slot;

    // use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
    RelCacheTable::relCache[relId] = relCacheBuffer;

    /****** Setting up Attribute Cache entry for the relation ******/

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    // let listHead be used to hold the head of the linked list of attrCache entries.
    AttrCacheEntry* attrCacheEntry = nullptr, *head = nullptr;

    int noOfAttributes = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;
    head = createAttrCacheLinkedList(noOfAttributes); // function to create linked list
    attrCacheEntry = head;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    /*iterate over all the entries in the Attribute Catalog corresponding to each
    attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
    care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
    corresponding to Attribute Catalog before the first call to linearSearch().*/
    for(int i = 0; i < noOfAttributes; i++)
    {
        /* let attrcatRecId store a valid record id an entry of the relation, relName,
        in the Attribute Catalog.*/
        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, attrVal, EQ);

        /* read the record entry corresponding to attrcatRecId and create an
        Attribute Cache entry on it using RecBuffer::getRecord() and
        AttrCacheTable::recordToAttrCatEntry(). */
        RecBuffer attrCatBlock(attrcatRecId.block);
        attrCatBlock.getRecord(attrCatRecord, attrcatRecId.slot);

        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
        /* update the recId field of this Attribute Cache entry to attrcatRecId.
        add the Attribute Cache entry to the linked list of listHead .*/
        attrCacheEntry->recId.block = attrcatRecId.block;
        attrCacheEntry->recId.slot = attrcatRecId.slot;
        // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
        attrCacheEntry = attrCacheEntry->next; // move to next entry in linked list
    }

    // set the relIdth entry of the AttrCacheTable to listHead.
    AttrCacheTable::attrCache[relId] = head;

    /****** Setting up metadata in the Open Relation Table for the relation******/

    // update the relIdth entry of the tableMetaInfo with free as false and
    // relName as the input.
    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName, relName);

    return relId;
}


int OpenRelTable::closeRel(int relId) {
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) { // prevent closing system relations
        return E_NOTPERMITTED;
    }

    if (relId < 0 || relId >= MAX_OPEN) { // check for valid rel-id
        return E_OUTOFBOUND;
    }

    if (tableMetaInfo[relId].free) { // check if relation is open by tableMetaInfo
        return E_RELNOTOPEN;
    }
    if(AttrCacheTable::attrCache[relId] == nullptr){ // check if relation is open by attrCache
        return E_RELNOTOPEN;
    }

    // free the memory allocated in the relation and attribute caches which was
    // allocated in the OpenRelTable::openRel() function
    free(RelCacheTable::relCache[relId]);
    AttrCacheEntry* head = AttrCacheTable::attrCache[relId];
    AttrCacheEntry* next = head->next;
    while(next != nullptr){
        free(head);
        head = next;
        next = next->next;
    }
    free(head);

    // update `tableMetaInfo` to set `relId` as a free slot
    tableMetaInfo[relId].free = true;
    // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
    RelCacheTable::relCache[relId] = nullptr;
    AttrCacheTable::attrCache[relId] = nullptr;
    return SUCCESS;
}