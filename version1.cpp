/**Uptil this point, data is getting entered in sorted order.
Next we have to implement splitting of the nodes, which will happen in version2.
*/
#include <iostream>
#include <fstream>
#include<stdio.h> 
#include<string.h>
#include <cstdlib>
using namespace std;

union charLong
  {
  long long lpart;
  char cpart[8];
  };
charLong cl;
void insert(string filename, string primary, int offset);
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
	
		int recsizeleaf,recsizenl,recperblockl,recperblocknl;
		int sizeinbufferl,sizeinbuffernl;
		recsizeleaf = keysize + 8;
		recsizenl = keysize + 8 + 8 + 8;
		sizeinbufferl = 1024 - sizeof(int) - sizeof(int) - sizeof(charLong); //flag,numrec,ptr to next leaf
		sizeinbuffernl = 1024 - sizeof(int) - sizeof(int);//flag,numrec
		recperblockl = (int)sizeinbufferl/recsizeleaf; 
		recperblocknl = (int)sizeinbuffernl/recsizenl;
		
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
	  	
	  	//remove the delim later
	  	char delim[5] = "end";
	  	strcpy(&buffer[bufptr], delim); //file name
		bufptr += strlen(delim);
	  	
		opindex.write(buffer,1024);
	  	opindex.close();
	  	memset(&buffer[0], 0, 1024);
		
		
		
		for(int i = 0; i < 5;i++)
		{
			insert("output1.indx",pkey[i],offset[i]);
		}
	}
	
		
		
		
}
void insert(string filename, string primary, int offset)
{
	//read metadata from input file
	fstream opindexr ("output1.indx", ios :: binary | ios::in|ios::out); 
	fstream opindexw ("output1.indx", ios::out| ios :: binary | ios::in ); //creating & opening index file in binary mode for writing
	
	char metadata[1024],node[1024];
	opindexr.read(&metadata[0],1024);
	char ipfile[1024];
	long rootptr;
	int bufptr = 0;
	int keysize,recperblockl,recperblocknl;
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
	//cout << "here " <<keysize<<" "<<rootptr<<" "<<ipfile<< endl;
	memset(&metadata[0], 0, 1024);
		
	
	//reading root node
	opindexr.seekg (rootptr, opindexr.beg);
	opindexr.read(&node[0],1024);
	//cout << strlen(node)<<endl;
	if (strlen(node) == 0) //first data element added
	{
		memset(&node[0], 0, 1024);
		int bufptr = 0;

		//TODO: add pointer to next leaf
		int leaf = 1; //isleaf
		memcpy(&node[bufptr],&leaf,sizeof(int));
		bufptr += sizeof(int);
		
		int numrec = 1;
		memcpy(&node[bufptr],&numrec,sizeof(int));
		bufptr += sizeof(int);
		
		strcpy(&node[bufptr], primary.c_str());
  		bufptr += strlen(primary.c_str());
		
		cl.lpart = offset;
		memcpy(&node[bufptr],cl.cpart, 8);
	  	bufptr += sizeof(charLong);
		  	
	  	opindexw.seekg(rootptr,opindexw.beg);
		opindexw.write(node,1024);
	  	memset(&node[0], 0, 1024);
		
	}
	else
	{
		bool flag = false;
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
		int isleaf;
		memcpy(&isleaf, &node[0], sizeof(int));

		int numrec;
		memcpy(&numrec, &node[sizeof(int)], sizeof(int));
		cout << numrec;
		
		if(isleaf == 1)
		{
			if(numrec < recperblockl)
			{
				int bufptr = sizeof(int)+sizeof(int);
				for(int i=0;i<numrec;i++)
				{
					char record[keysize+sizeof(charLong)];
					memcpy(&record[0],&node[bufptr],keysize+sizeof(charLong));
					char priold[keysize];
					memcpy(&priold[0],&record[0],keysize);
					if(priold > primary)
					{
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
				
				cout << node<<endl;
				
				opindexw.seekg(rootptr,opindexw.beg);
				opindexw.write(node,1024);
			  	memset(&node[0], 0, 1024);
			}
			else //time to split the node
			{
				//bufptr += ((int)(recperblockl+1)/2)*(keysize+sizeof(charLong))
//				char newnode[1024];
//				
//				int bufptr = 0;
//
//				int leaf = 1; //isleaf
//				memcpy(&newnode[bufptr],&leaf,sizeof(int));
//				bufptr += sizeof(int);
//				
//				int numrec = ;
//				memcpy(&node[bufptr],&numrec,sizeof(int));
//				bufptr += sizeof(int);
				
			}
		}
		
		
		
		
		
		
				
		
		
		
		
	}
	
	
		
	
	
}
  	
	
	
		
