#include "header.h"

/********* Global Variable **********/

/********* Blocks *********/
int8_t statusPages [671616];
int validPageCnt [5247]={0};

/********* Free Page Pool**********/
list<int> freePagePool;
list<int> freeBlcPool;
int bufferPoolBlock=0;

/********* Write Buffer **********/
list<buff> writeBuffer;

/********* Mapping ********/
map<int, int> logToPhy;
map<int, int> phyToLog;




int main()
{
    /********* Counter **********/
    int reQueueCnt=0;
    int deQueueCnt=0;
    int eraseCnt=0;
    int totalRq=0;
    int usrRdSize=0;
    int usrWrSize=0;
    int usrRdRq=0;
    int usrWrRq=0;
    int flashRdPg=0;
    int flashWrPg=0;
    int gcExtraRdPg=0;
    int gcExtraWrPg=0;
    int fullmergeExtraRdPg=0;

    //Initialiazation
    fill(statusPages, statusPages+610560, 1);
    fill(statusPages+610560, statusPages+671616, 0);
    fill(validPageCnt, validPageCnt+4770, 128);
    fill(validPageCnt+4770, validPageCnt+5247, -1);

    for(int i=0;i<610560;i++){
        logToPhy[i]=i;
        phyToLog[i]=i;
    }
    for(int i=4770;i<5247;i++){
        freeBlcPool.push_back(i);
    }

    
    string infileln;
    string a,b,c;
    int d,e;
    ifstream ifile("nb_20g_arrival.txt");
    if(ifile.is_open())
    {
        int cnt=1;
        while(ifile>>a>>b>>c>>d>>e)
        {
            totalRq++;
            if(e==0)continue; //Skip the size "0" request
            cnt++;
            int wrpage=((d+e-1)/64)-(d/64)+1;
            if (c=="W"){
                usrWrRq++;
                usrWrSize+=e;
                for(int i=0;i<wrpage;i++)
                {
                    int curLogAddr=floor(d/64);
                    int nsector=(64-(d%64));
                    int checkBuff=isInBuffByLog(curLogAddr);
                    if(checkBuff!=-1){
                        reQueue(checkBuff);
                        reQueueCnt++;
                    } 
                    else{
                        //If buffer full
                        if(writeBuffer.size()>=512)
                        {
                            //GC
                            if(freeBlcPool.size()<3){
                                int gcBlock=getLeastValidBlock();
                                if(gcBlock==-1){cout<<"No more place to gc"<<endl;exit(1);}
                                flashWrPg=flashWrPg+validPageCnt[gcBlock];
                                flashRdPg=flashRdPg+validPageCnt[gcBlock];
                                gcExtraWrPg=gcExtraWrPg+validPageCnt[gcBlock];
                                gcExtraRdPg=gcExtraRdPg+validPageCnt[gcBlock];

                                for(int j=(gcBlock*128);j<((gcBlock*128)+128);j++)
                                {
                                    int posBuff=isInBuffByPhy(j);
                                    if(statusPages[j]==1&&(posBuff==-1)){
                                        int oldLog=phyToLog[j];
                                        int newPg=getFreePg();
                                        if(oldLog!=-1){logToPhy[oldLog]=newPg;phyToLog[newPg]=oldLog;}
                                        else{cout<<"cant find in map error"<<endl;exit(1);}
                                    }
                                    else if(statusPages[j]==1&&(posBuff!=-1)){
                                        int newPg=getFreePg();
                                        buffChangePhy(newPg,posBuff);
                                    }
                                    statusPages[j]=0;                                   
                                }
                                freeBlcPool.push_back(gcBlock);
                                validPageCnt[gcBlock]=-1;
                                eraseCnt++;
                            }

                            //Evict victim page and write into flash
                            buff eviBuff=deQueue();
                            if(eviBuff.switchMerge){
                                int oldPhy=logToPhy[eviBuff.logAddr];
                                logToPhy[eviBuff.logAddr]=eviBuff.phyAddr;
                                phyToLog[eviBuff.phyAddr]=eviBuff.logAddr;
                                validPageCnt[(oldPhy/128)]--;
                                statusPages[oldPhy]=2;
                            }
                            else {
                                int oldPhy=logToPhy[eviBuff.logAddr];
                                int buffPhyAddr=getFreePg();
                                logToPhy[eviBuff.logAddr]=buffPhyAddr;
                                phyToLog[buffPhyAddr]=eviBuff.logAddr;
                                validPageCnt[(oldPhy/128)]--;
                                statusPages[oldPhy]=2;
                                validPageCnt[(eviBuff.phyAddr/128)]--;
                                statusPages[eviBuff.phyAddr]=2;
                                flashRdPg++;
                                flashWrPg++;
                                fullmergeExtraRdPg++;
                            }
                            deQueueCnt++;
                        }
                        
                        // cache into buffer
                        int buffPhyAddr=getFreePg();
                        if(nsector==64&&e>63){
                            enQueue(curLogAddr,buffPhyAddr,1);
                        }
                        else{
                            enQueue(curLogAddr,buffPhyAddr,0);
                        }

                    }
                    d=d+nsector; //starting sector
                    e=e-nsector; //sector left
                }
            }
            else{
                usrRdRq++;
                usrRdSize+=e;
                flashRdPg+=wrpage;
            }
        }
        
    }
    else cout<<"Fail to open file."<<endl;

    cout<<"Total Request Count: "<<totalRq<<endl;  
    cout<<"Total User Read Size: "<<usrRdSize<<" sectors = "<< usrRdSize/2<<"KB"<<endl;
    cout<<"Total User Write Size: "<<usrWrSize<<" sectors = "<< usrWrSize/2<<"KB"<<endl;
    cout<<"Total Flash Page Read Count: "<<flashRdPg<<endl;
    cout<<"Total Flash Page Write Count: "<<flashWrPg<<endl;
    cout<<"Total Flash Block Erase Count: "<<eraseCnt<<endl;
    
    cout<<"\n\n###################### EXTRA INFO ######################"<<endl;

    cout<<"Free Block Left: "<<freeBlcPool.size()<<endl;
    cout<<"GC: Extra Read="<<gcExtraRdPg<<" Extra Write="<<gcExtraWrPg<<endl;
    cout<<"Full Merge: Extra Read="<<fullmergeExtraRdPg<<" Write="<<(flashWrPg-gcExtraWrPg)<<endl;
    cout<<"User Read (Page): "<<(flashRdPg-gcExtraRdPg-fullmergeExtraRdPg)<<endl;

    cout<<"\nWrite Buffer:"<<endl;
    cout<<"Page cached in Buffer: "<<writeBuffer.size()<<endl;
    cout<<"Full and deQueue Cnt "<<deQueueCnt<<endl;
    cout<<"Hit and reQueue Cnt "<<reQueueCnt<<endl;

   // cout<<"\nValid Page Count of Every Block:\n"<<endl;
   // printBlockValidCnt();

    //cout<<"\nData cached in Buffer(Logical Address):\n"<<endl;
   // printBuffer();
    return 0;
}
