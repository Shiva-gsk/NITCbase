#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <bits/stdc++.h>
using namespace std;


void printSchema() {

	RecBuffer relCatBuffer(RELCAT_BLOCK);

  HeadInfo relCatHeader;

  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  // (we will implement these functions later)
  relCatBuffer.getHeader(&relCatHeader);


  for (int i = 0; i < relCatHeader.numEntries; i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    int attrBlockNum = ATTRCAT_BLOCK;
    while (attrBlockNum != -1) {
      RecBuffer attrCatBuffer(attrBlockNum);
      HeadInfo attrCatHeader;
      attrCatBuffer.getHeader(&attrCatHeader);

      int nextAttrBlock = attrCatHeader.rblock;

      for (int j = 0; j < attrCatHeader.numEntries; j++) {

        // declare attrCatRecord and load the attribute catalog entry into it
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);

        if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {
          const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
          printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
        }
      }

      attrBlockNum = nextAttrBlock;
    }
    printf("\n");
  }
}

void updateAttributeName(const char* relName, const char* oldAttrName, const char* newAttrName){
	bool relationFound = false;
	bool attributeUpdated = false;

	for(int attrBlockNum = ATTRCAT_BLOCK; attrBlockNum != -1; ){
		RecBuffer attrCatBuffer(attrBlockNum);
		HeadInfo attrCatHeader;
		attrCatBuffer.getHeader(&attrCatHeader);

		int nextAttrBlock = attrCatHeader.rblock;

		for(int recordIndex = 0; recordIndex < attrCatHeader.numEntries; recordIndex++){
			Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
			attrCatBuffer.getRecord(attrCatRecord, recordIndex);

			if(strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName) != 0)
				continue;

			relationFound = true;

			if(strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldAttrName) != 0)
				continue;

			strncpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newAttrName, ATTR_SIZE - 1);
			attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal[ATTR_SIZE - 1] = '\0';
			attrCatBuffer.setRecord(attrCatRecord, recordIndex);

			cout << "Updated attribute name from " << oldAttrName << " to " << newAttrName
			     << " in relation " << relName << "\n";
			attributeUpdated = true;
			break;
		}

		if(attributeUpdated)
			return;

		attrBlockNum = nextAttrBlock;
	}

	if(!relationFound)
		cout << "Relation " << relName << " not found\n";
	else
		cout << "Attribute " << oldAttrName << " not found in relation " << relName << "\n";
}

void printCatalog(){
  RelCatEntry *relCatBuf = new RelCatEntry();
  RelCacheTable::getRelCatEntry(RELCAT_RELID, relCatBuf);
  cout << "Relation: " << relCatBuf->relName << endl;

  for (int j = 0; j < relCatBuf->numAttrs; j++)


  {
    AttrCatEntry *attrCatBuf = new AttrCatEntry();
    AttrCacheTable::getAttrCatEntry(RELCAT_RELID, j, attrCatBuf);
    cout << "  " << attrCatBuf->attrName << ": " << attrCatBuf->attrType << endl;
  }

  RelCacheTable::getRelCatEntry(ATTRCAT_RELID, relCatBuf);
  cout << "Relation: " << relCatBuf->relName << endl;






  for (int j = 0; j < relCatBuf->numAttrs; j++)
  {
    AttrCatEntry *attrCatBuf = new AttrCatEntry();
    AttrCacheTable::getAttrCatEntry(ATTRCAT_RELID, j, attrCatBuf);
    cout << "  " << attrCatBuf->attrName << ": " << attrCatBuf->attrType << endl;
  }

  RelCacheTable::getRelCatEntry(2, relCatBuf);
  cout << "Relation: " << relCatBuf->relName << endl;

  for (int j = 0; j < relCatBuf->numAttrs; j++)
  {
    AttrCatEntry *attrCatBuf = new AttrCatEntry();
    AttrCacheTable::getAttrCatEntry(2, j, attrCatBuf);
    cout << "  " << attrCatBuf->attrName << ": " << attrCatBuf->attrType << endl;
}
}


int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  // create objects for the relation catalog and attribute catalog
  // printSchema();
  // updateAttributeName("Students", "Class", "Batch");
	// printSchema();
  printCatalog();

  return 0;
}