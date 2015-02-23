#include "BigQ.h"
#include <time.h>
#include <algorithm>
#include <stdlib.h>



/**
 ** This function will create a Run object and add it to the priority queue
 **/
void BigQ :: AddRunToQueue (int runSize, int pageOffset) {
    Run* run = new Run(runSize, pageOffset, &runsFile, sortord);
    runQueue.push(run);
}

/**
 ** Function to merge the runs into a single sorted file
 **/
void BigQ :: MergeRuns () {
    
    Run* run = new Run(&runsFile, sortord);
    Page page;
    
    //We will run this merge until the queue does not become empty. Queue empty means all the records have been taken care of.
    int i=0;
    while (!runQueue.empty()) {
        Record* record = new Record();
        run = runQueue.top();
        runQueue.pop();
            
        //first copy the top element(that was jst popped)
        record->Copy(run->firstRecord);
        outputPipe->Insert(record);
        if (run->GetFirstRecord() > 0)
            runQueue.push(run);
        
        delete record;
    }

    runsFile.Close();
    remove(fileName);
    
    outputPipe->ShutDown();
    delete run;
}


/**
 ** Global function comparer which will be used by sort to compare objects of type Record.
 **/

comparer::comparer(OrderMaker *order)
{
    sortord = order;
}
bool comparer::operator() (Record* left, Record* right) 
{
    ComparisonEngine comparisonEngine;
    int comparisonResult = 0;
    comparisonResult = comparisonEngine.Compare(left, right, sortord);
    
    if(comparisonResult<0)
        return true;
    else
        return false;
}


/**
 ** The constructor that will be called by the test
 **/
BigQ :: BigQ (Pipe &in, Pipe &out, OrderMaker &sortorder, int runlen) {
	// read data from in pipe sort them into runlen pages

    
    // construct priority queue over sorted runs and dump sorted data 
 	// into the out pipe
    sortord = &sortorder;
    inputPipe = &in;
    outputPipe = &out;
    runlength = runlen;
    totalPages = 1;
    //start the main thread from the constructor from where the sorting
    //will begin
    
    pthread_create(&workerThread, NULL, StartMainThread, (void *)this);
}



/**
 ** default constructor
 **/
BigQ :: BigQ () {
    runlength = 0;
    inputPipe = 0;
    outputPipe = 0;
    totalPages = 1;
}


/**
 ** copy constructor for the BigQ class
 **/
BigQ :: BigQ (const BigQ& big) {
    inputPipe = big.inputPipe;
    outputPipe = big.outputPipe;
    runlength = big.runlength;
    sortord = big.sortord;
    recordList = big.recordList;
    workerThread = big.workerThread;
    totalPages = big.totalPages;
}


/**
 ** Assignment operator for the BigQ class
 **/
BigQ& BigQ :: operator= (const BigQ& big) {
    inputPipe = big.inputPipe;
    outputPipe = big.outputPipe;
    runlength = big.runlength;
    recordList = big.recordList;
    workerThread = big.workerThread;
    sortord = big.sortord;
    totalPages = big.totalPages;
    return *this;
}


/**
 ** Destructor for the BigQ
 **/
BigQ::~BigQ () {
    
}



/**
 ** This function will write the 
 **/
bool BigQ :: WriteRunToFile (int runLocation) {
    
    Page* page = new Page();
    int recordListSize = recordList.size();
    
    Record* newRec = new Record();
    newRec->Copy(recordList[0]);
    delete newRec;
    
    int firstPageOffset = totalPages;
    int pageCounter = 1;
    
    //somewhere where it fails it will return a fase, that we will later use
    for (int i=0; i<recordListSize; i++) {
        Record* record = recordList[i];
        if ((page->Append(record)) == 0) {
            
            pageCounter++;                //to keep track of the number of pages for each run.
            
            runsFile.AddPage(page, totalPages) ;
            totalPages++;
            page->EmptyItOut();
            page->Append(record);
        }
        delete record;
    }
    
    //write the not full page to the file too.
    runsFile.AddPage(page, totalPages);
    totalPages++;
    page->EmptyItOut();
    
    //clear the recordlist after you're done using it
    recordList.clear();
    delete page;
    
    
    //Associate this location with the current run created which will be used as an offset to reference 
    //that run later on. This method will initialize an object of class Run which will keep track of the
    //run size and the page offset for each run.
    AddRunToQueue (recordListSize, firstPageOffset);
    return true;
}



void BigQ :: SortRecordList() {

    Page* page = new Page();
    int pageCounter = 0, recordCounter = 0, currentRunLocation = 0;
    Record* record = new Record();
    srand ( time(NULL) );
    fileName = new char(7);
    sprintf(fileName, "%d.txt", (int)workerThread);
    
    runsFile.Open(0, fileName);
    
    while (inputPipe->Remove(record)) {
        Record* copyOfRecord = new Record();
        copyOfRecord->Copy(record);
        recordCounter++;    //incrementing the record counter as the records keep coming.
        
        if(page->Append(record) == 0) {
            
            pageCounter++;
            if (pageCounter == runlength) {
                
                sort(recordList.begin(), recordList.end(), comparer(sortord));
                currentRunLocation = (runsFile.GetLength() == 0) ? 0 : (runsFile.GetLength() - 1);
                
                int recordListSize = recordList.size();
                
                //Write this list to file, at the specified Page location. After the run is written to file, it is cleared inside
                //this function itself
                WriteRunToFile (currentRunLocation); 
                
                pageCounter = 0;
            }
            page->EmptyItOut();
            page->Append(record);
        }
        
        //After inserting to the page, insert the record into the vector as well
        recordList.push_back(copyOfRecord);
        
    }
    
    
    //This is for the last run, which does not become completely full.
    if(recordList.size() > 0) {
        sort(recordList.begin(), recordList.end(), comparer(sortord));
        currentRunLocation = (runsFile.GetLength() == 0) ? 0 : (runsFile.GetLength() - 1);
        
        int recordListSize = recordList.size();
        //Write this list to file, at the specified Page location. After the run is written to file, it is cleared inside
        //this function itself
        WriteRunToFile (currentRunLocation);
        
        page->EmptyItOut();
    }

    delete record;
    delete page;    
}



/**
 ** Below are all the functions for the Run class
 **/

Run :: Run(int run_length, int page_offset, File *file, OrderMaker* order) {
    
    runSize = run_length;
    pageOffset = page_offset;
    firstRecord = new Record();
    runsFile=file;
    sortord = order;
    runsFile->GetPage(&topPage, pageOffset);
    GetFirstRecord();             //This will initialize the topPage and the firstRecord class variables.
}


Run :: Run (const Run& run) {
    firstRecord = run.firstRecord;
    pageOffset = run.pageOffset;
    runSize = run.runSize;
    topPage = run.topPage;
    runsFile = run.runsFile;
    sortord = run.sortord;
}


Run& Run :: operator= (const Run& run) {
    firstRecord = run.firstRecord;
    pageOffset = run.pageOffset;
    runSize = run.runSize;
    topPage = run.topPage;
    runsFile = run.runsFile;
    sortord = run.sortord;
    return *this;
}


Run :: Run(File *file, OrderMaker *order) {
    firstRecord = NULL;
    runsFile = file;
    sortord = order;
}


Run :: ~Run () {
    delete firstRecord;
    firstRecord = 0;
}


/**
 ** This function will return the current first record of the run from the page, and if the run has exhausted then it will
 ** return a NULL value.
 **/
int Run :: GetFirstRecord() {
    
    if(runSize <= 0) {
        return 0;
    }
    
    Record* record = new Record();
    
    //From the topPage page, get the first record, but if it returns 0, which means the page is empty, then increment the pageOffset by one, 
    //bring that page into memory and then get the first record from that page.
    if (topPage.GetFirst(record) == 0) {
        pageOffset++;
        runsFile->GetPage(&topPage, pageOffset);
        topPage.GetFirst(record);
    }
    
    //decrement the runSize as we extracted one record
    --runSize;
    
    //copy this record into the firstRecord field of the object.
    firstRecord->Consume(record);
    
    return 1;
}


/**
 ** Below are all the functions relevant to the Compare class.
 **/
Compare :: Compare () {

}

Compare :: Compare (OrderMaker *order)
{
    sortord = order;
}

Compare :: ~Compare () {

}

bool Compare :: operator() (Run* left, Run* right) {
    int comparisonResult = 0;
    ComparisonEngine comparisonEngine;
    
    comparisonResult = comparisonEngine.Compare(left->firstRecord, right->firstRecord, left->sortord);
    
    if (comparisonResult<0)
        return false;
    else
        return true;
}

