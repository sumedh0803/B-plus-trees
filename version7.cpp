/**
 * Written by Sumedh Sen for CS6360.004, assignment 6, starting November 18 , 2019.
 * NetID: SPS180006
 * */
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
void insert(string opfilename,string primary, int offset, long nodeptr);
char *sort(int bufptr,int keysize,int isleaf,int numrec,char *node,string primary,int offset,int &flag);
char* sortroot(int keysize,int isleaf,int numrec,long selflocr,string primary,long leftaddr,long rightaddr, char* oldroot);
void display(string opfilename,int offset,int keysize);
void displayroot(string opfilename,int offset,int keysize);
void insertnew(string opfilename,string record);
bool exists(string opfilename,string primary,long ptr);
long find(string opfilename,string primary,long ptr);
void list(string opfilename,string primary,long ptr,int displaycount);
int main (int argc, char** argv) 
{
	string ipfilename,opfilename,mode;
	int keysize,recptr,bufptr;
	int recsizeleaf,recsizenl,recperblockl,recperblocknl,sizeinbufferl,sizeinbuffernl; 
	long rootptr,writeloc;
	string line, tempk;
	int readsize = 0;
	int count = 0,tempof,pos;
	char record1[100],buffer[1024];
	long l1;
	
	mode = argv[1];
	
	if(mode == "-create")
	{
		//This segment creates an index file. It first calculates the number of records in the data file. 
		//THen, we store the primary keys and offset in 2 separate arrays. We then calculate the space available in the buffer to store our keys and offet,
		//after storing metadata of the node (int isleaf, integer values numrec (number of records in a node), location of the node itself and location of the next leaf)
		//Then we write the metadata at the beginning of the index file (input filename, keysize, pointer to root, max records in a leaf node and non leaf node each next write location in the index file
		//Lastly we insert each record in the index file. 
		ipfilename = argv[2];
		opfilename = argv[3];
		keysize = atoi(argv[4]);
		
		ifstream ipdata (ipfilename.c_str(),ios::binary); //opening data file in binary mode for reading
		fstream opindex (opfilename.c_str(), ios :: binary | ios :: out ); //creating & opening index file in binary mode for writing
		fstream opindexr (opfilename.c_str(), ios :: binary | ios::in|ios::out); 
		char key[keysize] = "";
		if (ipdata.is_open())
		{
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
			string ipfileblnkapp = ipfilename;
			int blankappend = 256 - ipfilename.size();
			for(i = 0;i<blankappend;i++)
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
		  	
		  	
			for(i = 0; i < count;i++)
			{
				
				opindexr.seekg(0,opindexr.beg);
				char metadata[1024];
				long rootptr,writeloc;
				int bufptr = 0;
				opindexr.read(&metadata[0],1024);			
				bufptr += 256 + sizeof(int);
				memcpy(cl.cpart, &metadata[bufptr], sizeof(charLong));
				rootptr = cl.lpart;				
				insert(opfilename,pkey[i],offset[i],rootptr);
			}
			
		}
		cout << "Index created successfully"<<endl;
	
	
	}
	else if(mode == "-find")
	{
		//First we read the metadata from the index file. Then we find the record in the index file. If found, read the data element from the input file.		
		opfilename = argv[2];
		string findpri = argv[3];
		
		fstream opindexr (opfilename.c_str(), ios :: binary | ios::in|ios::out);
		opindexr.seekg(0,opindexr.beg);
		char metadata[1024], ipfile[1024];;
		opindexr.read(&metadata[0],1024);
		bufptr = 0;
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
		
		//We remove the spaces that we had appended at the end of the file name.
		string newname = "";
		int i = 0;
		while(!isblank(ipfile[i]))
		{
			newname += ipfile[i];
			i++;
		}
		
		if (findpri.length() > keysize)
		{
			//truncate
			findpri = findpri.substr(0,keysize);
			
		}
		else
		{
			//append blanks
			string findpriblank = findpri;
			int blankappend = keysize - findpri.length();
			for(int i = 0;i<blankappend;i++)
			{
				findpriblank += " ";
			}
			findpri = findpriblank;
		}
		long pos = find(opfilename,findpri,rootptr);
		if(pos == -1)
			
		{
			cout  <<findpri<< " does not exist."<<endl;
		}
		else
		{
			//The record is found in the index. Read the data from the input file and print it.
			fstream opindexw (newname.c_str() ,ios :: binary | ios::in|ios::out); 
			opindexw.seekg(pos,opindexw.beg);
			string data;
			getline(opindexw,data);
			cout << "At "<<pos<<" record: "<<data<<endl;
			
		}
		
	}
	else if(mode == "-insert")
	{
		//We insert a new record in this section. The primary key and offet will also be added in the index file.
		opfilename = argv[2];
		string record = "";
		int noofargs = argc-2;
		for(int i=3;i<argc;i++)
		{
			record += argv[i];
			record += " ";
		}
		insertnew(opfilename,record);
		
	}
	else if(mode == "-list")
	{
		//In this section we list  a particular number of records from the input file. 
		opfilename = argv[2];
		string findpri = argv[3];
		int displaycount = atoi(argv[4]);
		
		//read the metadata.
		fstream opindexr (opfilename.c_str(), ios :: binary | ios::in|ios::out);
		opindexr.seekg(0,opindexr.beg);
		char metadata[1024], ipfile[1024];;
		opindexr.read(&metadata[0],1024);
		bufptr = 0;
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
		
		
		string newname = "";
		int i = 0;
		while(!isblank(ipfile[i]))
		{
			newname += ipfile[i];
			i++;
		}
		
		if (findpri.length() > keysize)
		{
			//truncate
			findpri = findpri.substr(0,keysize);
			
		}
		else
		{
			//append blanks
			string findpriblank = findpri;
			int blankappend = keysize - findpri.length();
			for(int i = 0;i<blankappend;i++)
			{
				findpriblank += " ";
			}
			findpri = findpriblank;
		}		
		list(opfilename,findpri,rootptr,displaycount);
		
	}
	else
	{
		cout<< "Invalid"<<endl;
	}
	return 0;
}
void insert(string opfilename, string primary, int offset, long nodeptr)
{
	
	fstream opindexr (opfilename.c_str(), ios :: binary | ios::in|ios::out); 
	fstream opindexw (opfilename.c_str(), ios::out| ios :: binary | ios::in ); 
	
	//read metadata
	char metadata[1024], ipfile[1024];
	int keysize,recperblockl,recperblocknl;
	long rootptr,writeloc;
	int bufptr = 0;
	opindexr.read(&metadata[0],1024);
	
	//reading metadata.
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
		
	//reading root node
	char node[1024];
	opindexr.seekg (nodeptr, opindexr.beg);
	opindexr.read(&node[0],1024);
	if (strlen(node) == 0) //first data element added here
	{
		memset(&node[0], 0, 1024);
		int bufptr = 0;

		int isleaf = 10;
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
		
		if(isleaf == 10)
		{
			long nextleaf;
			memcpy(cl.cpart, &node[bufptr], sizeof(charLong));
			nextleaf = cl.lpart;
			bufptr += sizeof(charLong);
			if(numrec < recperblockl) // data can be added in the present leaf node.
			{
				
				int bufptr = 2*sizeof(int)+2*sizeof(charLong);
				int flag;
				//add the new data element at the correct position in the current node. Then copy back all the elements in the original node.
				memcpy(&node[0],sort(bufptr,keysize,isleaf,numrec, node,primary,offset,flag),1024);	
				if (flag == 1)
				{
					//if data is added in the node, increase the number of records (numrec) and write it back at the correct position.
					numrec++;
					memcpy(&node[sizeof(int)],&numrec, sizeof(int));	
				}
				 
				//Write back the node in its original position	
				opindexw.seekg(selfloc,opindexw.beg);
				opindexw.write(node,1024);
			  	memset(&node[0], 0, 1024);
			}
			else //Capacity of the node is filled, so split it.
			{
				//tempx is used to store all the data from the original file and the new data.
				int tempxsize = 1024-2*sizeof(int)-2*sizeof(charLong)+keysize+sizeof(charLong);				
				char tempx[tempxsize];
				
				memcpy(&tempx[0],&node[bufptr],(keysize+sizeof(charLong))*recperblockl);
				int bufptr = 0;
				
				//sort the original data and the new data element
				int flag;
				memcpy(&tempx[0],sort(bufptr,keysize,isleaf,numrec,tempx,primary,offset,flag),tempxsize);
				bufptr = 0;
				
				//at this point, tempx has all the data in sorted order. including the new data.
				//now we have to copy half of the data into the original array and half in new.
				
				int leftcount = floor(recperblockl/2) + 1;
				int rightcount = recperblockl - leftcount + 1;
				long leftaddr,rightaddr;
				
				//initialising the right node
				bufptr = 0;
				char newnode[1024];
				
				isleaf = 10; //isleaf
				memcpy(&newnode[bufptr],&isleaf,sizeof(int));
				bufptr += sizeof(int);
				
				numrec = rightcount;
				memcpy(&newnode[bufptr],&numrec,sizeof(int));
				bufptr += sizeof(int);

				cl.lpart = writeloc;
				memcpy(&newnode[bufptr],cl.cpart,sizeof(charLong)); //self position
				bufptr += sizeof(charLong);
				rightaddr = writeloc;
				
				cl.lpart = nextleaf;
				memcpy(&newnode[bufptr],cl.cpart,sizeof(charLong)); //next leaf
				bufptr += sizeof(charLong);
								
				memcpy(&newnode[bufptr],&tempx[(leftcount)*(keysize+sizeof(charLong))],rightcount*(keysize+sizeof(charLong)));
				
				opindexr.seekg (writeloc, opindexr.beg);
				opindexr.write(newnode,1024);
				
				//resetting the left node.
				memset(&node[0], 0, 1024);
				bufptr = 0;

				isleaf = 10; //isleaf
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
				
				bufptr = 2*sizeof(int)+2*sizeof(charLong)+(leftcount-1)*(keysize+sizeof(charLong));
				
				char tempkey[100];
				memcpy(&tempkey,&node[bufptr],keysize);
				
				
				opindexw.seekg (selfloc, opindexw.beg);
				opindexw.write(node,1024);
				
				//updating next write location in metadata
			  	writeloc = writeloc + 1024;
			  	cl.lpart = writeloc;
			  	memcpy(&metadata[256+3*sizeof(int)+sizeof(charLong)],cl.cpart, 8);
			  	opindexw.seekg(0,opindexw.beg);
				opindexw.write(metadata,1024);				
				
				if (rootptr == selfloc)
				{
					//first data element is added in the root. i.e. leaf node is split for the 1st time and new root node is formed.
					char root[1024];
					bufptr = 0;
					isleaf = 2; //isleaf
					memcpy(&root[bufptr],&isleaf,sizeof(int));
					bufptr += sizeof(int);
					
					numrec = 1;
					memcpy(&root[bufptr],&numrec,sizeof(int));
					bufptr += sizeof(int);
	
					selfloc = writeloc;
					cl.lpart = selfloc;
					memcpy(&root[bufptr],cl.cpart,sizeof(charLong)); //self position
					bufptr += sizeof(charLong);
					
					memcpy(&root[bufptr],&tempx[(leftcount-1)*(keysize+sizeof(charLong))],keysize);
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
				else
				{
					//root node already exisits. sort and add the new root element (new primary key in the root node)
					opindexw.seekg(rootptr,opindexw.beg);
					char root[1024];
					bufptr = 0;
					opindexw.read(&root[0],1024);
					int isleafr,numrecr;
					long selflocr;
					
					memcpy(&isleafr,&root[bufptr],sizeof(int));
					bufptr += sizeof(int);
					
					memcpy(&numrecr,&root[bufptr],sizeof(int));
					bufptr += sizeof(int);
					
					memcpy(&cl.cpart,&root[bufptr],sizeof(charLong));
					selflocr = cl.lpart;
					bufptr += sizeof(charLong);
					
					memcpy(&root[0],sortroot(keysize,isleafr,numrecr,selflocr,tempkey,leftaddr,rightaddr, root),1024);
					opindexw.seekg(rootptr,opindexw.beg);
					opindexw.write(root,1024);	
				}
			}
		}
		else if (isleaf == 2)
		{
			//root node encountered first.
			char key[100],nextkey[100];
			for (int i = 0;i<numrec+1;i++)
			{
				memcpy(&key,&node[bufptr],keysize);
				memcpy(&nextkey,&node[bufptr+keysize+2*sizeof(charLong)],keysize);
				if (primary < key)
				{
					// go to the left node from root node
					long left;
					memcpy(&left,&node[bufptr+keysize],sizeof(charLong));
					insert(opfilename,primary,offset,left);
					break;	
				}
				else if(i == numrec-1 && primary > key)
				{
					// go to the right most node from root node
					long right;
					memcpy(&right,&node[bufptr+keysize+sizeof(charLong)],sizeof(charLong));
					insert(opfilename,primary,offset,right);
					break;
				}
				else if(primary > key && primary < nextkey)
				{
					// go to the middle node from root node
					long right;
					memcpy(&right,&node[bufptr+keysize+sizeof(charLong)],sizeof(charLong));
					insert(opfilename,primary,offset,right);
					break;
				}
				else
				{
					bufptr += keysize + 2*sizeof(charLong);
				}
			}
		}
	}
}

void display(string opfilename,int offset, int keysize)
{
	//displays the primary keys serially from the leaf nodes
	fstream opindexr (opfilename.c_str(), ios :: binary | ios::in|ios::out); 
	char node[1024];
	opindexr.seekg (offset, opindexr.beg);
	opindexr.read(&node[0],1024);
	int bufptr = sizeof(int);
	long next;
	int numrec;
	
	memcpy(&numrec,&node[bufptr],sizeof(int));
	bufptr += sizeof(int)+sizeof(charLong);
	memcpy(&next,&node[bufptr],sizeof(charLong));
	bufptr += sizeof(charLong);
	for(int i =0;i< numrec;i++)
	{
		char pri[keysize];
		memcpy(&pri,&node[bufptr],keysize);
		cout <<pri <<endl;
		bufptr += keysize+sizeof(charLong);
	}
	
	if(next != 0)
	{
		display(opfilename,next,keysize);			
	}
	return;
}

void displayroot(string opfilename, int offset,int keysize)
{
	//displays the primary keys serially from the root nodes
	fstream opindexr (opfilename.c_str(), ios :: binary | ios::in|ios::out); 
	char node[1024];
	opindexr.seekg (offset, opindexr.beg);
	opindexr.read(&node[0],1024);
	int bufptr = sizeof(int);
	int numrec;
	memcpy(&numrec,&node[bufptr],sizeof(int));
	bufptr += sizeof(int) + sizeof(charLong);
	for(int i =0;i< numrec;i++)
	{
		char pri[keysize];
		long left, right;
		memcpy(&pri[0],&node[bufptr],keysize);
		cout <<pri;
		bufptr += keysize;
		memcpy(&left,&node[bufptr],keysize);
		bufptr += 8;
		cout <<" "<<left;
		memcpy(&right,&node[bufptr],keysize);
		bufptr += 8;
		cout <<" "<<right<<endl;
	}
	
}	
char* sort(int bufptr,int keysize,int isleaf,int numrec,char *node,string primary,int offset,int &flag)
{
	//this function adds the new data element at the coorect positon in the leaf node
	flag = 0;
	for(int i=0;i<numrec;i++)
	{
		
		char record[keysize];
		memcpy(&record[0],&node[bufptr],keysize);
		char priold[keysize];
		memcpy(&priold[0],&record[0],keysize);
		if (priold == primary)
		{
			flag = 0;
			return node;
		}
		if(priold > primary)
		{
			flag = 1;//sorting has taken place
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
			
		}
		else
		{
			bufptr += keysize + sizeof(charLong);
		}
	}
	if (flag == 0)
	{
		flag = 1;
		strcpy(&node[bufptr], primary.c_str());
  		bufptr += strlen(primary.c_str());
		
		cl.lpart = offset;
		memcpy(&node[bufptr],cl.cpart, 8);
	  	bufptr += sizeof(charLong);
	}
	return node;
	
}
	
char* sortroot(int keysize,int isleaf,int numrec,long selflocr,string primary,long leftaddr,long rightaddr, char* oldroot)
{
	//this function adds the new data element at the correct positon in the root node
	int bufptr = 0;
	memcpy(&oldroot[bufptr],&isleaf,sizeof(int));
	bufptr += sizeof(int);
	
	numrec++;
	memcpy(&oldroot[bufptr],&numrec,sizeof(int));
	bufptr += sizeof(int);

	cl.lpart = selflocr;
	memcpy(&oldroot[bufptr],cl.cpart,sizeof(charLong)); //self position
	bufptr += sizeof(charLong);
	
	bool flag = false;
	for (int i =0;i<numrec-1;i++)
	{
		char record[keysize];
		memcpy(&record[0],&oldroot[bufptr],keysize);
		char priold[keysize];
		memcpy(&priold[0],&record[0],keysize);
		if(priold > primary)
		{
			flag = true;//sorting has taken place
			char temp[1024];
			memcpy(&temp[0],&oldroot[bufptr],(keysize+2*sizeof(charLong))*(numrec-1-i)); //copy old data
			
			//insert new data
			strcpy(&oldroot[bufptr], primary.c_str());
	  		bufptr += strlen(primary.c_str());
			
			cl.lpart = leftaddr;
			memcpy(&oldroot[bufptr],cl.cpart, 8);
		  	bufptr += sizeof(charLong);
		  	
		  	cl.lpart = rightaddr;
			memcpy(&oldroot[bufptr],cl.cpart, 8);
		  	bufptr += sizeof(charLong);
		  	
		  	//copy back old data
		  	memcpy(&oldroot[bufptr],&temp[0],(keysize+2*sizeof(charLong))*(numrec-1-i));
		  	bufptr = 2*sizeof(int) + sizeof(charLong) + (i+1)*(keysize+2*sizeof(charLong)) + keysize;
		  	memcpy(&oldroot[bufptr],&rightaddr,sizeof(charLong));
		  	break;		
		}
		else
		{
			bufptr += keysize + 2*sizeof(charLong);
		}
		
	}
	if (flag == false)
	{
		strcpy(&oldroot[bufptr], primary.c_str());
  		bufptr += strlen(primary.c_str());
		
		cl.lpart = leftaddr;
		memcpy(&oldroot[bufptr],cl.cpart, 8);
	  	bufptr += sizeof(charLong);
	  	
  		cl.lpart = rightaddr;
		memcpy(&oldroot[bufptr],cl.cpart, 8);
	  	bufptr += sizeof(charLong);
	}
	return oldroot;
}	
		
void insertnew(string opfilename,string record)
{
	
	string data = record;
	data += '\n';
	int len = record.length();
	fstream opindexr (opfilename.c_str(), ios :: binary | ios::in|ios::out); 
	char tempname[256],metadata[1024];
	opindexr.read(&metadata[0],1024);
	int bufptr = 0;
	memcpy(&tempname,&metadata[bufptr],256);
	bufptr += 256;
	
	string newname = "";
	int i = 0;
	while(!isblank(tempname[i]))
	{
		newname += tempname[i];
		i++;
	}
	int keysize = 0;
	
	memcpy(&keysize,&metadata[bufptr],sizeof(int));
	int tempsize = keysize;
	bufptr += sizeof(int);	
	
	long rootptr;
	memcpy(&rootptr,&metadata[bufptr],sizeof(charLong));
	bufptr += sizeof(charLong);	
	
	string primary;
	primary = record.substr(0,tempsize);
		
	if(!exists(opfilename,primary,rootptr))
	{	
		//record doesnt exist. so add it
		fstream opindexw (newname.c_str(), ios :: binary | ios::in|ios::out); 
		opindexw.seekg(0,ios_base::end);
		int pos;
		pos = opindexw.tellp();
		insert(opfilename,primary,pos,rootptr);
		opindexw.write(data.c_str(),data.length());
		cout << "At "<<pos<<" record entered: "<<data<<endl;
	}		
}
bool exists(string opfilename, string primary,long ptr)
{
	//checks if the primary key exists in the index file or not
	fstream opindexr (opfilename.c_str(), ios :: binary | ios::in|ios::out); 
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
	
	//reading root node
	char node[1024];
	opindexr.seekg (ptr, opindexr.beg);
	opindexr.read(&node[0],1024);
	
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
	
	if(isleaf == 10)
	{
		long nextleaf;
		memcpy(cl.cpart, &node[bufptr], sizeof(charLong));
		nextleaf = cl.lpart;
		bufptr += sizeof(charLong);
		
		for (int i = 0;i<numrec;i++)
		{
			char pri[100];
			memcpy(&pri,&node[bufptr],keysize);
			if(pri == primary)
			{
				cout << "Record with key "<<primary<<" already exists"<<endl;
				return true;
			}
			else
			{
				bufptr += keysize + sizeof(charLong);
			}
		}
		
		return false;
	}
	else
	{
		char key[100],nextkey[100];
		
		for (int i = 0;i<numrec+1;i++)
		{
			memcpy(&key,&node[bufptr],keysize);
			memcpy(&nextkey,&node[bufptr+keysize+2*sizeof(charLong)],keysize);
			if (primary < key)
			{
				long left;
				memcpy(&left,&node[bufptr+keysize],sizeof(charLong));
				if(exists(opfilename,primary,left))
					return true;
				break;	
			}
			else if(i == numrec-1 && primary > key)
			{
				
				long right;
				memcpy(&right,&node[bufptr+keysize+sizeof(charLong)],sizeof(charLong));
				if(exists(opfilename,primary,right))
					return true;
				break;
			}
			else if(primary > key && primary < nextkey)
			{
				long right;
				memcpy(&right,&node[bufptr+keysize+sizeof(charLong)],sizeof(charLong));
				if(exists(opfilename,primary,right))
					return true;
				break;
			}
			else
			{
				bufptr += keysize + 2*sizeof(charLong);
			}
		}
		return false;		
	}
}
long find(string opfilename, string primary,long ptr)
{
	//searches for the primary key in the index file
	fstream opindexr (opfilename.c_str(), ios :: binary | ios::in|ios::out); 
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
	
	//reading root node
	char node[1024];
	opindexr.seekg (ptr, opindexr.beg);
	opindexr.read(&node[0],1024);
	
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
	
	if(isleaf == 10)
	{
		long nextleaf;
		memcpy(cl.cpart, &node[bufptr], sizeof(charLong));
		nextleaf = cl.lpart;
		bufptr += sizeof(charLong);
		
		for (int i = 0;i<numrec;i++)
		{
			char pri[100];
			memcpy(&pri,&node[bufptr],keysize);
			if(pri == primary)
			{
				long offset;
				memcpy(&offset,&node[bufptr+keysize],sizeof(charLong));
				return offset;
			}
			else
			{
				bufptr += keysize + sizeof(charLong);
			}
		}
		
		return -1;
	}
	else
	{
		char key[100],nextkey[100];
		
		for (int i = 0;i<numrec+1;i++)
		{
			memcpy(&key,&node[bufptr],keysize);
			memcpy(&nextkey,&node[bufptr+keysize+2*sizeof(charLong)],keysize);
			if (primary < key)
			{
				long left;
				memcpy(&left,&node[bufptr+keysize],sizeof(charLong));
				long offset;
				offset = find(opfilename,primary,left);
				return offset;
			}
			else if(i == numrec-1 && primary > key)
			{
				
				long right;
				memcpy(&right,&node[bufptr+keysize+sizeof(charLong)],sizeof(charLong));
				long offset;
				offset = find(opfilename,primary,right);
				return offset;
			}
			else if(primary > key && primary < nextkey)
			{
				long right;
				memcpy(&right,&node[bufptr+keysize+sizeof(charLong)],sizeof(charLong));
				long offset;
				offset = find(opfilename,primary,right);
				return offset;
			}
			else
			{
				bufptr += keysize + 2*sizeof(charLong);
			}
		}
		return -1;		
	}
}
void list(string opfilename, string primary,long ptr,int displaycount)
{
	//lists a specific number of data records from the input file.
	fstream opindexr (opfilename.c_str(), ios :: binary | ios::in|ios::out); 
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
	
	string newname = "";
	int i = 0;
	while(!isblank(ipfile[i]))
	{
		newname += ipfile[i];
		i++;
	}
	//reading root node
	char node[1024];
	opindexr.seekg (ptr, opindexr.beg);
	opindexr.read(&node[0],1024);
	
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
	
	if(isleaf == 10)
	{
		long nextleaf;
		memcpy(cl.cpart, &node[bufptr], sizeof(charLong));
		nextleaf = cl.lpart;
		bufptr += sizeof(charLong);

		for (int i = 0;i<numrec;i++)
		{
			char pri[100];
			memcpy(&pri,&node[bufptr],keysize);
			if(pri == primary)
			{
				if (numrec - i < displaycount)
				{
					for(int j=0;j<numrec-i;j++)
					{
						long offset;
						memcpy(&offset,&node[bufptr+keysize],sizeof(charLong));
						fstream opindexw (newname.c_str(), ios :: binary | ios::in|ios::out); 
						opindexw.seekg(offset,opindexw.beg);
						string data;
						getline(opindexw,data);
						cout << data << endl;
						bufptr += keysize + sizeof(charLong);
					}
					opindexr.seekg (nextleaf, opindexr.beg);
					if(nextleaf != 0)
					{
						opindexr.read(&node[0],1024);
						bufptr = 2*sizeof(int) + 2*sizeof(charLong);
						for(int j=0;j<displaycount - numrec - i;j++)
						{
							long offset;
							memcpy(&offset,&node[bufptr+keysize],sizeof(charLong));
							fstream opindexw (newname.c_str(), ios :: binary | ios::in|ios::out); 
							opindexw.seekg(offset,opindexw.beg);
							string data;
							getline(opindexw,data);
							cout << data << endl;
							bufptr += keysize + sizeof(charLong);	
						}	
					}
					else return;
					return;	
				}
				else
				{
					for(int j=0;j<displaycount ; j++)
					{
						long offset;
						memcpy(&offset,&node[bufptr+keysize],sizeof(charLong));
						fstream opindexw (newname.c_str(), ios :: binary | ios::in|ios::out); 
						opindexw.seekg(offset,opindexw.beg);
						string data;
						getline(opindexw,data);
						cout << data << endl;
						bufptr += keysize + sizeof(charLong);		
					}
					return;	
				}
			}
			else if(pri > primary)
			{
				if (numrec - i < displaycount)
				{
					for(int j=0;j<numrec-i;j++)
					{
						long offset;
						memcpy(&offset,&node[bufptr+keysize],sizeof(charLong));
						fstream opindexw (ipfile, ios :: binary | ios::in|ios::out); 
						opindexw.seekg(offset,opindexw.beg);
						string data;
						getline(opindexw,data);
						cout << data << endl;
						bufptr += keysize + sizeof(charLong);
						
					}
					opindexr.seekg (nextleaf, opindexr.beg);
					if(nextleaf != 0)
					{
						opindexr.read(&node[0],1024);
						bufptr = 2*sizeof(int) + 2*sizeof(charLong);
						for(int j=0;j<displaycount - numrec - i;j++)
						{
							long offset;
							memcpy(&offset,&node[bufptr+keysize],sizeof(charLong));
							fstream opindexw (ipfile, ios :: binary | ios::in|ios::out); 
							opindexw.seekg(offset,opindexw.beg);
							string data;
							getline(opindexw,data);
							cout << data << endl;
							bufptr += keysize + sizeof(charLong);
							
						}
						
						
					}
					else return;
					return;
					
					
				}
				else
				{
					for(int j=0;j<displaycount ; j++)
					{
						long offset;
						memcpy(&offset,&node[bufptr+keysize],sizeof(charLong));
						fstream opindexw (ipfile, ios :: binary | ios::in|ios::out); 
						opindexw.seekg(offset,opindexw.beg);
						string data;
						getline(opindexw,data);
						cout << data << endl;
						bufptr += keysize + sizeof(charLong);
						
					}
					return;
					
				}
				
					
			}
			else
			{
				bufptr += keysize + sizeof(charLong);
			}
		}
		return;
	}
	else
	{
		char key[100],nextkey[100];
		
		for (int i = 0;i<numrec+1;i++)
		{
			memcpy(&key,&node[bufptr],keysize);
			memcpy(&nextkey,&node[bufptr+keysize+2*sizeof(charLong)],keysize);
			if (primary < key)
			{
				long left;
				memcpy(&left,&node[bufptr+keysize],sizeof(charLong));
				list(opfilename,primary,left,displaycount);
				return;	
			}
			else if(i == numrec-1 && primary > key)
			{	
				long right;
				memcpy(&right,&node[bufptr+keysize+sizeof(charLong)],sizeof(charLong));
				list(opfilename,primary,right,displaycount);
				return;
			}
			else if(primary > key && primary < nextkey)
			{
				long right;
				memcpy(&right,&node[bufptr+keysize+sizeof(charLong)],sizeof(charLong));
				list(opfilename,primary,right,displaycount);
				return;
			}
			else
			{
				bufptr += keysize + 2*sizeof(charLong);
			}
		}
		return;		
	}
}
