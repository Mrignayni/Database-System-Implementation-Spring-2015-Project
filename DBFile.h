#ifndef DBFILE_H
#define DBFILE_H

#include <sys/types.h>
#include "TwoWayList.h"
#include "Record.h"
#include "Schema.h"
#include "File.h"
#include "Comparison.h"
#include "ComparisonEngine.h"

typedef enum {heap, sorted, tree} fType;
typedef enum {OPEN, CLOSED} fStatus;

// stub DBFile header..replace it with your own DBFile.h 

class DBFile {

private:
	Page page;
	File file;
    off_t pgIndex;
    Record *myRec;
    Page currpage;
    int currpgindex;
    int currrecindex;
    int status;
    

public:
	DBFile ();  //Constructor
	~DBFile (); //Destructor

	
	int Create (char *fpath, fType file_type, void *startup);
	int Open (char *fpath);
	int Close ();

	void Load (Schema &myschema, char *loadpath);

	void MoveFirst ();
	void Add (Record &addme);
	int GetNext (Record &fetchme);
	int GetNext (Record &fetchme, CNF &cnf, Record &literal);
    
    int LastPageIndex(); //gets the index of the page
    int LastPageOfFileIndex(); //get the index of the last page of the file
    
    

};
#endif
