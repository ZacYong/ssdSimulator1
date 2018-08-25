#include "header.h"

int isInBuffByLog(int logAddr){
    int i=0;
    for ( list<buff>::iterator it=writeBuffer.begin(); it != writeBuffer.end(); ++it){
        if(it->logAddr==logAddr)return i;
        i++;
    }
    return -1;
}
void enQueue(int log,int phy,bool merge){
    buff a;
    a.logAddr=log;
    a.switchMerge=merge;
    a.phyAddr=phy;
    writeBuffer.push_back(a);
}
void reQueue(int pos)
{
    std::list<buff>::iterator it;
    it = writeBuffer.begin();
    advance(it,pos);  
    writeBuffer.splice(writeBuffer.end(),writeBuffer,it);
}
void buffChangePhy(int phy,int pos)
{
    std::list<buff>::iterator it;
    it = writeBuffer.begin();
    advance(it,pos);  
    it->phyAddr=phy;
}
buff deQueue()
{
    buff temp;
    temp=writeBuffer.front();
    writeBuffer.pop_front();
    return temp;
}
void printBuffer()
{
    int i=0;
   
    for ( list<buff>::iterator it=writeBuffer.begin(); it != writeBuffer.end(); ++it){
         cout<<i<<" "<<it->logAddr<<" "<<it->switchMerge<<endl;
         i++;
    }
     
}
void printBlockValidCnt()
{
    for(int i=0;i<5247;i++){
         cout<<i<<" "<<validPageCnt[i]<<endl;
    }
}
int getFreePg()
{
     if(freePagePool.size()==0){
        int freeBlc=freeBlcPool.front();
        bufferPoolBlock=freeBlc;
        freeBlcPool.pop_front();
        validPageCnt[freeBlc]=0;
        for(int j=(freeBlc*128);j<((freeBlc*128)+128);j++)freePagePool.push_back(j);
    }
    int buffPhyAddr=freePagePool.front();
    freePagePool.pop_front();
    validPageCnt[(buffPhyAddr/128)]++;
    statusPages[buffPhyAddr]=1;
    return buffPhyAddr;
}
int isInBuffByPhy(int phy){
    int i=0;
    for ( list<buff>::iterator it=writeBuffer.begin(); it != writeBuffer.end(); ++it){
        if(it->phyAddr==phy)return i;
        i++;
    }
    return -1;
}
int getLeastValidBlock()
{
    int min_pos=-1;
    int min=128;
    for(int i=0;i<5247;i++){
        if(validPageCnt[i]<min&&validPageCnt[i]!=-1&&i!=bufferPoolBlock){
            min_pos=i;
            min=validPageCnt[i];
        }
    }
    return min_pos;
}