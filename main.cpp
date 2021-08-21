#include "wfilesearcher.hpp"

int quitCode = 0;

int main(int argc, char ** argv) {
  if(!WFSearchInit())
    return 0xFF;
  while(WFSearchHandleEvents())
    ;
  return quitCode;
}