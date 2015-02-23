//#define OPEN 1

#include "TwoWayList.h"
#include "Record.h"
#include "Schema.h"
#include "File.h"
#include "Comparison.h"
#include "ComparisonEngine.h"
#include "DBFile.h"
#include "Defs.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
//fStatus status;


DBFile::DBFile () {
    //fStatus status;
    status = 0;
    pgIndex = (off_t)-1;
    currrecindex = 0;
    currpgindex = 0;

}

DBFile::~DBFile (){
}

int DBFile::Create (char *f_path, fType f_type, void *startup) {

    //file = new File();
    file.Open(0,f_path);
    status	=1;
    //fStatus status = fStatus.OPEN;
    return 1;
}

void DBFile::Load (Schema &f_schema, char *loadpath) {
        // cout << "DBFile::Load: Loading the binary file." << endl;
        Record temp_rec;
        Page temp_pg;
        FILE* FILEobj = fopen (loadpath, "r");
        if(FILEobj == NULL)
			return;
        while (temp_rec.SuckNextRecord (&f_schema, FILEobj) == 1) {
                        if (temp_pg.Append(&temp_rec) == 0){
                                // current page is full
                                pgIndex++;
                                file.AddPage(&temp_pg, pgIndex);
                                temp_pg.EmptyItOut();
                                temp_pg.Append(&temp_rec);
                                /*if (!page.Append(&myRec)) {
                                         cerr << "Error: record couldn't be added even on new page." << endl;
                                }*/
                        }
        }
        pgIndex++;
        file.AddPage(&temp_pg, pgIndex);
}

/*void DBFile::Load (Schema &f_schema, char *loadpath) {
    
    FILE *fReading = fopen(loadpath, "r");
    
    Record rec;
    while (rec.SuckNextRecord(&f_schema, fReading)==1) {
        Add(rec);
    }
    
    fclose(fReading);
}*/

void DBFile::Add (Record &rec) { //incomplete
    
    Page temp_pg;
    pgIndex = file.GetLength() - 1;
    file.GetPage(&temp_pg, pgIndex);
    pgIndex--;
    if(temp_pg.Append(&rec) != 1)
    {
		pgIndex++;
		file.AddPage(&temp_pg, pgIndex);
		temp_pg.EmptyItOut();
		temp_pg.Append(&rec);
    }
    pgIndex++;
    file.AddPage(&temp_pg, pgIndex);
}    
    /*Page latest;
    int indexOfLast;
    
    indexOfLast = file.GetLength() -2 ;
    if( indexOfLast==0) //checking if we are at the last page already
    {
        file.GetPage(&latest, 0);
    }
    else //if we are not at the last page execute the below to get to the last page
    {
        file.GetPage(&latest, indexOfLast);
    }
    
    //after the coreect page position has been found
    //check if it actually has the room for the new record
    
    if(latest.Append(&rec)!= 1)
    {	//cout<<"";
        //file.AddPage(&latest, indexOfLast);
        latest.EmptyItOut();
	latest.Append(&rec);
	file.AddPage(&latest, indexOfLast +1);
    }
    else
    {
	file.AddPage(&latest, indexOfLast);
    }*/



int DBFile::Open (char *f_path) {
    
    if(f_path==NULL)
    {
        cout << "Null Path. Exiting!" <<endl;
    return 0;
    }

    if (status!=1) {
        
        file.Open(1,f_path);
        
        status = 1;
        return 1;
    } else {
        cout << "File is already open." <<endl;
        return 0;
    }

}

void DBFile::MoveFirst () {

	file.GetPage(&currpage, (off_t)0);
	myRec = currpage.movepointertofirst();
	currrecindex = 0;

}

int DBFile::Close () {

    if(status==0)
    {
        cout << "File is not open." << endl;
        return 0;
    }
    else
    {
		status = 0;
        return(file.Close());
        }
}




/*int DBFile::LastPageIndex()
{
    if (file.GetLength()==0) {
        return (off_t) 0;
    } else {
        return file.GetLength() - (off_t)2;
    }
}


int DBFile::LastPageOfFileIndex()
{

    if (file.GetLength()==0) {
        return (off_t) 0;
    } else {
        return file.GetLength() - (off_t) 1;
    }
}*/


int DBFile::GetNext (Record &fetchme) {

    if(currpgindex == 0 || page.GetFirst(&fetchme) == 0)
	{
		if(currpgindex >= file.GetLength() - 1)
			return 0;
		file.GetPage(&page, currpgindex);
		currpgindex++;
		page.GetFirst(&fetchme);
	}
	return 1;
}
	/*else
        if (pgIndex < LastPageOfFileIndex() - (off_t) 1) {
            pgIndex = pgIndex+1;
            page.EmptyItOut();
            file.GetPage(&page, pgIndex);
            getNextSuccess = page.GetFirst(&fetchme);
        } else {
            getNextSuccess=0;
        }

	return getNextSuccess;*/


int DBFile::GetNext (Record &fetchme, CNF &cnf, Record &literal) {
    
    ComparisonEngine compareEng;
    
    while (GetNext(fetchme) == 1) {
        if (compareEng.Compare(&fetchme, &literal, &cnf)==1) {
            return 1;
        }
    }
    return 0;
}

