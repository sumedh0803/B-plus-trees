/**the code perfectly creates the index file. however, this happens only for 
a fixed keylength of 15. Try to make it dynamic. also test if the root
node ever splits.*/
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
void insert(string primary, int offset, long nodeptr);
char *sort(int bufptr,int keysize,int isleaf,int numrec,char *node,string primary,int offset,int &flag);
char* sortroot(int keysize,int isleaf,int numrec,long selflocr,string primary,long leftaddr,long rightaddr, char* oldroot);
void display(int offset);
void displayroot(int offset);
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
	fstream opindexr ("output1.indx", ios :: binary | ios::in|ios::out); 
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
	  	
	  	
		for(i = 0; i < 909;i++)
		{
			
			opindexr.seekg(0,opindexr.beg);
			char metadata[1024];
			long rootptr,writeloc;
			int bufptr = 0;
			opindexr.read(&metadata[0],1024);			
			bufptr += 256 + sizeof(int);
			memcpy(cl.cpart, &metadata[bufptr], sizeof(charLong));
			rootptr = cl.lpart;
			//cout << keysize << " " <<rootptr << " "<<recperblockl << " "<<recperblocknl<<endl;
			if(i == 88)cout << pkey[i]<<endl;
			insert(pkey[i],offset[i],rootptr);
		}
		display(1024);
		cout << endl;
		//display(2048);
		displayroot(3072);
	}		
}
void insert(string primary, int offset, long nodeptr)
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

	//cout << nodeptr << " nodeptr"<<endl;
	
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
		//if (primary == "44332837200542A") cout << numrec<<" "<<primary << " "<<selfloc<<endl;
		
		if(isleaf == 10)
		{
			//cout << "leaf"<<endl;
			long nextleaf;
			memcpy(cl.cpart, &node[bufptr], sizeof(charLong));
			nextleaf = cl.lpart;
			bufptr += sizeof(charLong);
			if(numrec < recperblockl)
			{
				
				int bufptr = 2*sizeof(int)+2*sizeof(charLong);
				//sorting here
				int flag;
				memcpy(&node[0],sort(bufptr,keysize,isleaf,numrec, node,primary,offset,flag),1024);	
				if (flag == 1)
				{
					
					numrec++;
					memcpy(&node[sizeof(int)],&numrec, sizeof(int));
					
				}
						
				opindexw.seekg(selfloc,opindexw.beg);
				opindexw.write(node,1024);
			  	memset(&node[0], 0, 1024);
			}
			else //time to split the node
			{
//				cout << primary<<endl;
				int tempxsize = 1024-2*sizeof(int)-2*sizeof(charLong)+keysize+sizeof(charLong);				
				char tempx[tempxsize];
				
				memcpy(&tempx[0],&node[bufptr],(keysize+sizeof(charLong))*recperblockl);
				int bufptr = 0;
				
				//sorting here
				int flag;
				memcpy(&tempx[0],sort(bufptr,keysize,isleaf,numrec,tempx,primary,offset,flag),tempxsize);
				bufptr = 0;
				
				
				
				//at this point, tempx has all the data in sorted order. including the new data.
				//now we have to copy half of the data into the original array and half in new.
				
				int leftcount = floor(recperblockl/2) + 1;
				//cout << "leftc: "<<leftcount<<endl;
				int rightcount = recperblockl - leftcount + 1;
				//cout << "rightc: "<<rightcount<<endl;
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
				
				
				//resetting the left node. write the new leaf and then add here.
				memset(&node[0], 0, 1024);
				bufptr = 0;

				//TODO: add pointer to next leaf
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
				char tempkey[15];
				memcpy(&tempkey,&node[bufptr],keysize);
				//cout << "newk "<<tempkey<<endl;
				
				
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
					//making the new root node
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
					//cout << "updting root"<<endl;
					//cout << rootptr <<endl;
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
					
					//cout << isleafr << " "<<numrecr << " "<<selflocr<<endl;
								
//					char newkey[keysize];
//					string newk;
//					//cout <<"newk"<< newk<<endl;
//					memcpy(&newkey[0],&node[2*sizeof(int)+sizeof(charLong)+(leftcount-2)*(keysize+sizeof(charLong))],keysize);
//					cout <<"newk"<< newkey<<endl;
					
					memcpy(&root[0],sortroot(keysize,isleafr,numrecr,selflocr,tempkey,leftaddr,rightaddr, root),1024);
					opindexw.seekg(rootptr,opindexw.beg);
					opindexw.write(root,1024);
					
				}
						
				
				
				
				
				
				
			}
		}
		else if (isleaf == 2)
		{
			//cout << "root"<<endl;
			
			char key[15],nextkey[15];
			for (int i = 0;i<numrec+1;i++)
			{
				memcpy(&key,&node[bufptr],keysize);
				//cout << primary <<endl;
				memcpy(&nextkey,&node[bufptr+keysize+2*sizeof(charLong)],keysize);
				if (primary < key)
				{
					long left;
					memcpy(&left,&node[bufptr+keysize],sizeof(charLong));
					insert(primary,offset,left);
					break;
					
					
					
				}
				else if(i == numrec-1 && primary > key)
				{
					
					long right;
					memcpy(&right,&node[bufptr+keysize+sizeof(charLong)],sizeof(charLong));
//					if(primary == "67240570100754A")
//						cout << "go right "<<numrec<<" "<<right<<endl;
					insert(primary,offset,right);
					break;
				}
				else if(primary > key && primary < nextkey)
				{
					//cout << "go mid"<<endl;
					long right;
					memcpy(&right,&node[bufptr+keysize+sizeof(charLong)],sizeof(charLong));
					insert(primary,offset,right);
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

void display(int offset)
{
	fstream opindexr ("output1.indx", ios :: binary | ios::in|ios::out); 
	char node[1024];
	opindexr.seekg (offset, opindexr.beg);
	opindexr.read(&node[0],1024);
	int bufptr = sizeof(int);
	long next;
	int numrec;
	
	memcpy(&numrec,&node[bufptr],sizeof(int));
	cout <<"numrec at "<<offset << " is "<<numrec<<endl;
	bufptr += sizeof(int)+sizeof(charLong);
	memcpy(&next,&node[bufptr],sizeof(charLong));
	bufptr += sizeof(charLong);
	for(int i =0;i< numrec;i++)
	{
		char pri[15];
		memcpy(&pri[0],&node[bufptr],15);
		cout <<pri <<endl;
		bufptr += 23;
	}
	
	if(next != 0)
	{
		cout << "next: "<<next<<endl<<endl;
		display(next);			
	}
	return;
}

void displayroot(int offset)
{
	fstream opindexr ("output1.indx", ios :: binary | ios::in|ios::out); 
	char node[1024];
	opindexr.seekg (offset, opindexr.beg);
	opindexr.read(&node[0],1024);
	//cout << node;
	int bufptr = sizeof(int);
	int numrec;
	memcpy(&numrec,&node[bufptr],sizeof(int));
	bufptr += sizeof(int) + sizeof(charLong);
	//cout << numrec;
	for(int i =0;i< numrec;i++)
	{
		char pri[15];
		long left, right;
		memcpy(&pri[0],&node[bufptr],15);
		cout <<pri;
		bufptr += 15;
		memcpy(&left,&node[bufptr],15);
		bufptr += 8;
		cout <<" "<<left;
		memcpy(&right,&node[bufptr],15);
		bufptr += 8;
		cout <<" "<<right<<endl;
	}
	
}	
char* sort(int bufptr,int keysize,int isleaf,int numrec,char *node,string primary,int offset,int &flag)
{
	//cout << tempint<<endl;
	flag = 0;
	
	for(int i=0;i<numrec;i++)
	{
		
		char record[keysize];
		memcpy(&record[0],&node[bufptr],keysize);
		char priold[keysize];
		memcpy(&priold[0],&record[0],keysize);
		if (priold == primary)
		{
			//cout << i<<" "<< priold << " "<<primary<<endl;
			//cout << "Repeated value: "<<primary<<endl;
			flag = 0;
			return node;
		}
		if(priold > primary)
		{
			//cout << priold << " " <<primary<<endl;
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
			
			//memcpy(&node[bufptr+keysize+sizeof(charLong)], &node[bufptr], keysize+sizeof(charLong));
			
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
	int bufptr = 0;
	//if(primary == "72679285000406A") cout <<"pri "<< numrec<<endl;
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
		//if(primary == "72679285000406A") cout << priold << " " <<primary<<endl;
		if(priold > primary)
		{
			//if(primary == "72679285000406A") cout << priold << " " <<primary<<endl;
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
			
			//memcpy(&node[bufptr+keysize+sizeof(charLong)], &node[bufptr], keysize+sizeof(charLong));
			
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
		
