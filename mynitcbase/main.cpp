#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  // StaticBuffer buffer;
  // OpenRelTable cache;
  unsigned char buffer[BLOCK_SIZE];
  Disk::readBlock(buffer, 0);
  for (int i = 0; i < 20; i++) {
    std::cout << (int)buffer[i] << " ";
  }
  std::cout << std::endl;

  return 0;
  // return FrontendInterface::handleFrontend(argc, argv);
}