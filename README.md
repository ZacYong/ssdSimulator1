# SSD Simple Simulation

## Requirement

1. 單通道
2. 每次只要處理1個request
3. Page mapping
4. GC policy
- Greedy，每次回收valid page最少的block
- 統一在剩下2個free block的時候觸發GC

5. 參數設定
- physical size: 22007513088 bytes (overprovision約10%)
- logical size:  20006830080 bytes 
- block size:  4MB (128 pages)
- page size:  32KB
- write buffer size: 16MB
- - flush機制: page-level LRU

6. input file format
- 1061000000 JUNK R 31816558 8
simulator只需要拿每個request後面三項就好，(R/W)代表這個request是要做(讀取/寫入)，第四個代表request起始的sector，第五個代表從起始sector開始要對幾個sector操作
sector size = 512 bytes
- 模擬只會改變mapping跟flash的狀態，沒有實際的data讀寫
- trace file: https://drive.google.com/file/d/15dAZ5VvgCM4y3ZtpQ1WuQbPamfyHmSIj/view

7. output file format
- total request count
- total user read size
- - 總共讀了多少data(單位用KB)
- total user write size
- - 總共寫了多少data(單位用KB)
- total flash page read count
- total flash page write count
- total flash block erase count 

8. initial state
- 所有logical page都是valid -> 所有write request都會update

## SSD Properties
1 sector (512 bytes)
1 page (32KB) = 64 sectors
1 block (4MB) = 128 pages = 8192 sectors

SSD:
physical size: 
22007513088 bytes = 42983424 sectors = 671616 pages = 5247 blocks
logical size: 
20006830080 bytes = 39075840 sectors = 610560 pages = 4770 blocks
write buffer size: 
16MB = 32768 sectors = 512 pages = 4 blocks

## Program parameter
1. Table records # of valid page each Block
-> int validPageCnt [5247]

2. Mapping Table(log->phy)
-> map<int, int> logToPhy [610560]

3. Mapping Table(phy->log)
-> map<int, int> phyToLog [671616]

4. Status of each page(0 free 1 valid 2 invalid)
-> int_8 statusPages [671616]

5. List of free blocks (Block number)
-> list<int> freeBlcPool

6. List of free pages (Physical Page address)
(if empty, fill with 128 free pages of victim block from freeBlcPool) 
-> list<int> freePagePool

7. List of buffer page structure
-> list<buff> writeBuffer;
- struct buff {
    int logAddr=-1; //logical address
    int phyAddr=-1; //physical address
    bool switchMerge=0; //0:full merge 1:switch merge
};

## Input
Dataset 3142854 requests

## Algorithm

![](https://i.imgur.com/AexVl33.png)

### Remarks:
1. Initialization
-    Default: all logical is writen and valid, i just simply mapped (log i -> phy i) i from 0 to 610559, statusPages[i]=1;
-    the rest of physical page(610560->671616) marked free ,statusPages[i]=0;
-    validPageCnt[0...4770]=128, validPageCnt[4770...5247]=-1
-    since block [4770...5247]is free, i pushed them into freeBlcPool

2. Calculate X page(s) to R/W
- Since the beginning sector might not be the first sector of the page, the request might cross more than (n/64)
- for example 
-  ![](https://i.imgur.com/PmdTiLk.png)

3. Garbage Collection
- if free block less than 3, GC is triggered
-  gcBlock=getLeastValidBlock()
-  let Y be the number of valid page in gcBlock
-  iter each page of gcBlock: move the valid page to free page, remap, mark free
-  gcBlock is free and push back to freeBlcPool
-  there will be Y extra pages to read and write

4. Enqueue currWLog into WriteBuffer
- Before enqueue the write request, there will be a classification to classify the page whether is full merge or switch merge. If switch merge, it will no extra pages to read or write. Else, it will cost 1  exta read and 1 extra write operation.
- if it start from the first sector and there are more than 63 sector left, it will be a switch merge. Else, full merge.

## Results

![](https://i.imgur.com/N9fEAns.png)

### Remarks
1. Total Flash Page Read Count = UserRead + GCRead(Extra) + FullMergeRead(Extra)
- 9165088 = 2610437 + 5707688 + 846963
- 100% = 28.48% + 62.28% + 9.24%
2. Total Flash Page Write Count = FullMergeWrite + GCWrite(Extra) 
- 6554651 = 846963 + 5707688
- 100% = 12.92% + 87.08%
3. Full and deQueue Cnt is the times of buffer's full and evict a page
4. Hit and reQueue Cnt is the times of page hit and requeue the page

## Analysis 
#### To ensure the correctness of the simulator:
1. I sum up all the valid page count of the block after the simulation.
- the initial valid page count is (4770 logical blocks * 128 page)= 610560
- the total is 611072 = 610560 + 512(write buffer page)
- As the result, the mapping and status merchanism worked well

2. I plotted a graph of page number against times of the page W requested. Then, I highlight the page which cached in buffer with the yellow lines.
![](https://i.imgur.com/0OHU794.png)
- Source: https://i.imgur.com/0OHU794.png
- From the graph, we can observed that hot data are cached in the buffer after the simulation. It proved that the LRU buffer worked perfectly.

#### From the percentage pie chart below:
Due to 90% of ssd is written and only 10% free space left, the SSD spent much workload on GC.  
![](https://i.imgur.com/gIEYqBG.png)

![](https://i.imgur.com/Jllkxum.png)

## Source Code
https://github.com/ZacYong/ssdSimulator1
