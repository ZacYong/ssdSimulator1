#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <cstdint>
#include <list>
#include <map>
#include <time.h>
#include <algorithm> 
using namespace std;

struct buff{
    int logAddr=-1;
    int phyAddr=-1;
    bool switchMerge=0;
};

extern int8_t statusPages [671616];
extern int validPageCnt [5247];
extern list<int> freePagePool;
extern list<int> freeBlcPool;
extern map<int, int> logToPhy;
extern map<int, int> phyToLog;
extern int bufferPoolBlock;
extern list<buff> writeBuffer;


int isInBuffByLog(int logAddr);
void enQueue(int log,int phy,bool merge);
void reQueue(int pos);
void buffChangePhy(int phy,int pos);
buff deQueue();
void printBuffer();
void printBlockValidCnt();
int getFreePg();
int isInBuffByPhy(int phy);
int getLeastValidBlock();