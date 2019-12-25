/**Until this point. data gets split. root gets formed. data in both the nodes is sorted
now, next we have to check the value in root node, compare it with out new pri key. 
and accordingly go either to left or to right. also, code is repetitive. try to make new smaller functions*/
#include <iostream>
#include <fstream>
#include<stdio.h> 
#include<string.h>
#include <cstdlib>
#include <cmath>
using namespace std;

union charLong
  {
  long long lpart;
  char cpart[8];
  };
charLong cl;
void insert(string filename, string primary, int offset);
void display(int offset);
int main (int argc, char** argv) 
{
	string ipfilename,opfilename,mode;
	int keysize,recptr,bufptr;
	string line, tempk;
	int readsize = 0;
	int count = 0,tempof,pos;
	char record1[100],buffer[1024];
	long l1;
	keysize = 15;
	ifstream ipdata ("CS6360Asg5TestDataA.txt",ios::binary); //opening data file in binary mode for reading
	fstream opindex ("output1.indx", ios :: binary | ios :: out ); //creating & opening index file in binary mode for writing
	
	//getting all primay keys and their offsets
  	char key[keysize] = "";
	if (ipdata.is_open())
	{
		//counting the number of records
		while (getline(ipdata, line))
			count++;
		
		string pkey[count];
		int offset[count];
		
		//reseting the position of pointer to start of file
		ipdata.clear();
		ipdata.seekg(0, ios::beg);
		
		pos = ipdata.tellg();
		int i = 0;
		while (ipdata.read(key, sizeof(key)))
		{
			pkey[i] = key;
			offset[i++] = pos;
			getline(ipdata, line);
			pos = ipdata.tellg();
		}
		ipdata.close();
	
		int recsizeleaf,recsizenl,recperblockl,recperblocknl,sizeinbufferl,sizeinbuffernl; 
		recsizeleaf = keysize + 8; //key + offset
		recsizenl = keysize + 8 + 8;//key+left+right
		sizeinbufferl = 1024 - sizeof(int) - sizeof(int) - sizeof(charLong)-sizeof(charLong); //flag,numrec,ptr to next leaf,self location
		sizeinbuffernl = 1024 - sizeof(int) - sizeof(int)-sizeof(charLong);//flag,numrec,self location
		recperblockl = floor(sizeinbufferl/recsizeleaf); 
		recperblocknl = floor(sizeinbuffernl/recsizenl);
		
		//writing metadata
		ipfilename = "CS6360Asg5TestDataA.txt";
		string ipfileblnkapp = ipfilename;
		int blankappend = 256 - ipfilename.size();
		for(int i = 0;i<blankappend;i++)
		{
			ipfileblnkapp += " ";
		}
		bufptr = 0;
		strcpy(buffer, ipfileblnkapp.c_str()); //file name
		bufptr += strlen(ipfileblnkapp.c_str());
		
		
		memcpy(&buffer[bufptr],&keysize, sizeof(int));
	  	bufptr += sizeof(int);
	  	
	  	cl.lpart = 1024; //pointer to root
		memcpy(&buffer[bufptr],cl.cpart, 8);
	  	bufptr += sizeof(charLong);
	  	
	  	
		memcpy(&buffer[bufptr],&recperblockl,sizeof(int));
		bufptr += sizeof(int);

		memcpy(&buffer[bufptr],&recperblocknl,sizeof(int));
		bufptr += sizeof(int);
		
		cl.lpart = 1024; //next write location
		memcpy(&buffer[bufptr],cl.cpart, 8);
	  	bufptr += sizeof(charLong);
	  	
		opindex.write(buffer,1024);
	  	opindex.close();
	  	memset(&buffer[0], 0, 1024);
		
		
		
		for(int i = 0; i < 44;i++)
		{
			insert("output1.indx",pkey[i],offset[i]);
		}
		
		display(2048);
	}		
}
void insert(string filename, string primary, int offset)
{
	//read metadata from input file
	fstream opindexr ("output1.indx", ios :: binary | ios::in|ios::out); 
	fstream opindexw ("output1.indx", ios::out| ios :: binary | ios::in ); //creating & opening index file in binary mode for writing
	
	//read metadata
	char metadata[1024], ipfile[1024];
	int keysize,recperblockl,recperblocknl;
	long rootptr,writeloc;
	int bufptr = 0;
	opindexr.read(&metadata[0],1024);
	
	strncpy(ipfile, &metadata[0], 256);
	bufptr += 256;
	memcpy(&keysize, &metadata[bufptr], sizeof(int));
	bufptr += sizeof(int);
	memcpy(cl.cpart, &metadata[bufptr], sizeof(charLong));
	rootptr = cl.lpart;
	bufptr += sizeof(charLong);
	memcpy(&recperblockl, &metadata[bufptr], sizeof(int));
	bufptr += sizeof(int);
	memcpy(&recperblocknl, &metadata[bufptr], sizeof(int));
	bufptr += sizeof(int);
	memcpy(cl.cpart, &metadata[bufptr], sizeof(charLong));
	writeloc = cl.lpart;
	
	//cout << keysize << " " <<rootptr << " "<<recperblockl << " "<<recperblocknl<<endl;

	
	
	//reading root node
	char node[1024];
	opindexr.seekg (rootptr, opindexr.beg);
	opindexr.read(&node[0],1024);
	if (strlen(node) == 0) //first data element added here
	{
		memset(&node[0], 0, 1024);
		int bufptr = 0;

		int isleaf = 1;
		memcpy(&node[bufptr],&isleaf,sizeof(int));
		bufptr += sizeof(int);
		
		int numrec = 1;
		memcpy(&node[bufptr],&numrec,sizeof(int));
		bufptr += sizeof(int);
		
		cl.lpart = rootptr; //self location
		memcpy(&node[bufptr],cl.cpart, 8);
	  	bufptr += sizeof(charLong);
	  	
	  	cl.lpart = 0; //next leaf: currently set to 0
		memcpy(&node[bufptr],cl.cpart, 8);
	  	bufptr += sizeof(charLong);
		
		strcpy(&node[bufptr], primary.c_str());
  		bufptr += strlen(primary.c_str());
		
		cl.lpart = offset;
		memcpy(&node[bufptr],cl.cpart, 8);
	  	bufptr += sizeof(charLong);
		  	
	  	opindexw.seekg(rootptr,opindexw.beg);
		opindexw.write(node,1024);
	  	memset(&node[0], 0, 1024);
	  	
	  	//updating next write location in metadata
	  	writeloc = rootptr + 1024;
	  	cl.lpart = writeloc;
	  	memcpy(&metadata[256+3*sizeof(int)+sizeof(charLong)],cl.cpart, 8);
	  	opindexw.seekg(0,opindexw.beg);
		opindexw.write(metadata,1024);
	}
	else
	{
		
		//not the first record. so, read the meta data of index,
		// read meta data of the node (i.e. number of records and boolean isleaf
		//check for nuber of records. if it is less than the limit,
		//then no problem. else make 2 new buffers -> one for root, one
		//for 1/2 of the records. shift half the records directy to the new
		//buffer. set left and right pointers in root.
		//then, go to root, read the key, accordingly go to left of right
		// make space for the new record by performing linear search.
		//insert new record and write the buffers back to file
		//
		bufptr = 0;
		int isleaf;
		memcpy(&isleaf, &node[bufptr], sizeof(int));
		bufptr += sizeof(int);

		int numrec;
		memcpy(&numrec, &node[bufptr], sizeof(int));
		bufptr += sizeof(int);
	
		
		long selfloc;
		memcpy(cl.cpart, &node[bufptr], sizeof(charLong));
		selfloc = cl.lpart;
		bufptr += sizeof(charLong);
		
		long nextleaf;
		memcpy(cl.cpart, &node[bufptr], sizeof(charLong));
		nextleaf = cl.lpart;
		bufptr += sizeof(charLong);
		
		
		if(isleaf == 1)
		{
			if(numrec < recperblockl)
			{
				bool flag = false;
				int bufptr = 2*sizeof(int)+2*sizeof(charLong);
				for(int i=0;i<numrec;i++)
				{
					
					char record[keysize];
					memcpy(&record[0],&node[bufptr],keysize);
					char priold[keysize];
					memcpy(&priold[0],&record[0],keysize);
					if(priold > primary)
					{
						//cout << priold << " " <<primary<<endl;
						flag = true;//sorting has taken place
						char temp[1024];
						memcpy(&temp[0],&node[bufptr],(keysize+sizeof(charLong))*(numrec-i)); //copy old data
						
						//insert new data
						strcpy(&node[bufptr], primary.c_str());
				  		bufptr += strlen(primary.c_str());
						
						cl.lpart = offset;
						memcpy(&node[bufptr],cl.cpart, 8);
					  	bufptr += sizeof(charLong);
					  	
					  	//copy back old data
					  	memcpy(&node[bufptr],&temp[0],(keysize+sizeof(charLong))*(numrec-i));
					  	break;
						
						//memcpy(&node[bufptr+keysize+sizeof(charLong)], &node[bufptr], keysize+sizeof(charLong));
						
					}
					else
					{
						bufptr += keysize + sizeof(charLong);
					}
				}
				if (flag == false)
				{
					strcpy(&node[bufptr], primary.c_str());
			  		bufptr += strlen(primary.c_str());
					
					cl.lpart = offset;
					memcpy(&node[bufptr],cl.cpart, 8);
				  	bufptr += sizeof(charLong);
				}
				
				numrec++;
				memcpy(&node[sizeof(int)],&numrec, sizeof(int));
				
				opindexw.seekg(selfloc,opindexw.beg);
				opindexw.write(node,1024);
			  	memset(&node[0], 0, 1024);
			}
			else //time to split the node
			{
				/**Make a temp node, of size larger than original node. 
				keep the size such that itll allocate one more data and pointer element. copy the 
				original node in this new node. insert the new data at the appropriate location using the algo
				used above. after inserting, copy the 1st half of the data in original node(copy 1024-sizeof(int)-sizeof(int)
				bytes to remove old data. write left node. get address of write location. write right node,
				get address of write location.
				make another node for root. copy last element from left node in root. add left and right address in root node
				*/
				char newnode[1024],tempx[1024-2*sizeof(int)-2*sizeof(charLong)+keysize+sizeof(charLong)];
				
				memcpy(&tempx[0],&node[bufptr],(keysize+sizeof(charLong))*recperblockl);
				int bufptr = 0;
				bool flag = false;
				for(int i=0;i<numrec;i++) //sorting the records
				{
					char record[keysize+sizeof(charLong)];
					memcpy(&record[0],&tempx[bufptr],keysize+sizeof(charLong));
					char priold[keysize];
					memcpy(&priold[0],&record[0],keysize);
					if(priold > primary)
					{
						flag = true;//sorting has taken place
						char temp[1024];
						memcpy(&temp[0],&tempx[bufptr],(keysize+sizeof(charLong))*(numrec-i)); //copy old data
						
						//insert new data
						strcpy(&tempx[bufptr], primary.c_str());
				  		bufptr += strlen(primary.c_str());
						
						cl.lpart = offset;
						memcpy(&tempx[bufptr],cl.cpart, 8);
					  	bufptr += sizeof(charLong);
					  	
					  	//copy back old data
					  	memcpy(&tempx[bufptr],&temp[0],(keysize+sizeof(charLong))*(numrec-i));
					  	break;
						
						//memcpy(&node[bufptr+keysize+sizeof(charLong)], &node[bufptr], keysize+sizeof(charLong));
						
					}
					else
					{
						bufptr += keysize + sizeof(charLong);
					}
				}
				
				//at this point, tempx has all the data in sorted order. including the new data.
				//now we have to copy half of the data into the original array and half in new.
				
				int leftcount = floor(recperblockl/2) + 1;
				//cout << "leftc: "<<leftcount<<endl;
				int rightcount = recperblockl - leftcount + 1;
				//cout << "rightc: "<<rightcount<<endl;
				long leftaddr,rightaddr;
				
				//initialising the right node
				bufptr = 0;
				//TODO: add pointer to next leaf
				isleaf = 1; //isleaf
				memcpy(&newnode[bufptr],&isleaf,sizeof(int));
				bufptr += sizeof(int);
				
				numrec = rightcount;
				memcpy(&newnode[bufptr],&numrec,sizeof(int));
				bufptr += sizeof(int);

				cl.lpart = writeloc;
				memcpy(&newnode[bufptr],cl.cpart,sizeof(charLong)); //self position
				bufptr += sizeof(charLong);
				rightaddr = writeloc;
				
				cl.lpart = 0;
				memcpy(&newnode[bufptr],cl.cpart,sizeof(charLong)); //next leaf
				bufptr += sizeof(charLong);
								
				memcpy(&newnode[bufptr],&tempx[leftcount*(keysize+sizeof(charLong))],rightcount*(keysize+sizeof(charLong)));
				
				opindexr.seekg (writeloc, opindexr.beg);
				opindexr.write(newnode,1024);
				
				
				//resetting the left node. write the new leaf and then add here.
				memset(&node[0], 0, 1024);
				bufptr = 0;

				//TODO: add pointer to next leaf
				isleaf = 1; //isleaf
				memcpy(&node[bufptr],&isleaf,sizeof(int));
				bufptr += sizeof(int);
				
				numrec = leftcount;
				memcpy(&node[bufptr],&numrec,sizeof(int));
				bufptr += sizeof(int);
				
				cl.lpart = selfloc;
				memcpy(&node[bufptr],cl.cpart,sizeof(charLong));
				bufptr += sizeof(charLong);
				leftaddr = selfloc;
				
				cl.lpart = writeloc;
				memcpy(&node[bufptr],cl.cpart,sizeof(charLong)); //next leaf
				bufptr += sizeof(charLong);
				
				memcpy(&node[bufptr],&tempx[0],leftcount*(keysize+sizeof(charLong)));
				opindexw.seekg (selfloc, opindexw.beg);
				opindexw.write(node,1024);
				
				
				//updating next write location in metadata
			  	writeloc = writeloc + 1024;
			  	cl.lpart = writeloc;
			  	memcpy(&metadata[256+3*sizeof(int)+sizeof(charLong)],cl.cpart, 8);
			  	opindexw.seekg(0,opindexw.beg);
				opindexw.write(metadata,1024);				
				
				//making the new root node
				char root[1024];
				bufptr = 0;
				isleaf = 0; //isleaf
				memcpy(&root[bufptr],&isleaf,sizeof(int));
				bufptr += sizeof(int);
				
				numrec = 1;
				memcpy(&root[bufptr],&numrec,sizeof(int));
				bufptr += sizeof(int);

				selfloc = writeloc;
				cl.lpart = selfloc;
				memcpy(&root[bufptr],cl.cpart,sizeof(charLong)); //self position
				bufptr += sizeof(charLong);
				
				memcpy(&root[bufptr],&node[(leftcount-1)*keysize+sizeof(charLong)+2*sizeof(int)+2*sizeof(charLong)],keysize);
				bufptr += keysize;
				
				cl.lpart = leftaddr;
				memcpy(&root[bufptr],cl.cpart,sizeof(charLong));
				bufptr += sizeof(charLong);
				
				cl.lpart = rightaddr;
				memcpy(&root[bufptr],cl.cpart,sizeof(charLong));
				bufptr += sizeof(charLong);
				
				opindexw.seekg(selfloc,opindexw.beg);
				opindexw.write(root,1024);				
				
				
				//updating next write location in metadata
			  	writeloc = writeloc + 1024;
			  	cl.lpart = writeloc;
			  	memcpy(&metadata[256+3*sizeof(int)+sizeof(charLong)],cl.cpart, 8);
			  	opindexw.seekg(0,opindexw.beg);
				opindexw.write(metadata,1024);
				
				//updating root location in metadata
				cl.lpart = selfloc;
				memcpy(&metadata[256+sizeof(int)],cl.cpart, 8);
			  	opindexw.seekg(0,opindexw.beg);
				opindexw.write(metadata,1024);
				
				
				
			}
		}
		
	}
	
	
		
	
	
}

void display(int offset)
{
	fstream opindexr ("output1.indx", ios :: binary | ios::in|ios::out); 
	char node[1024];
	opindexr.seekg (offset, opindexr.beg);
	opindexr.read(&node[0],1024);
	int bufptr = sizeof(int);
	int numrec;
	memcpy(&numrec,&node[bufptr],sizeof(int));
	bufptr += sizeof(int) + 2*sizeof(charLong);
	
	for(int i =0;i< numrec;i++)
	{
		char pri[15];
		memcpy(&pri[0],&node[bufptr],15);
		cout <<pri <<endl;
		bufptr += 23;
	}
	
} 	
	
	
		
