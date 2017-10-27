#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

// system configuration
// total 592
// used 83
// free 204
// cache 304
// available 422
#define MAXREAD 1048576*100
#define MAXLEN 1048576*100
#define MAXROUND 1000

// function define
void load_round(int sig);
void dump_to_file();
void analyze();

// IO variable
FILE *fp=NULL;
FILE *logfile=NULL;
unsigned char *buff=NULL;

// Time variable
struct timeval tt1,tt2;
double ttimeuse;

// Msg transform
double *listen = NULL;
bool *decoded = NULL;
char *msg = NULL;
int round_count = 0;
double gaps = 0.18;
double prevalue = 0;
bool prestatus = false;
bool begin_listen = false; // change after first invoke
bool begin_capture = false;
bool record_need = false;
bool end_capture = false;
bool prefix_done = false;
int prefix_count = 0;
int bit_count = 0;
int char_count = 0;


int main(int argc, char *argv[])
{

	if(argc<2)
	{
		printf("input file name");
		exit(1);
	}
	if( (fp=fopen(argv[1],"rb")) == NULL)
	{
		printf("Cannot open file!");
		exit(1);
	}
	if((logfile=fopen("log.txt","w"))==NULL)
	{
		printf("Failed open log");
		exit(1);
	}

	listen = (double*)malloc(MAXROUND*sizeof(double));
	if(!listen)
	{
		printf("failed alloc listen");
		exit(1);
	}

	decoded = (bool*)malloc(MAXROUND*sizeof(bool));
	if(!decoded)
	{
		printf("failed alloc decoded");
		exit(1);
	}

	msg = (char*)malloc(MAXROUND*sizeof(char));
	if(!msg)
	{
		printf("failed alloc msg");
		exit(1);
	}

	buff = (unsigned char*)malloc(MAXLEN*sizeof(unsigned char));
	if(!buff)
	{
		printf("failed alloc");
		exit(1);
	}

	// set clock signal, invoke the listening function each sec.
	gettimeofday(&tt1,NULL);
	int res = 0;
	signal(SIGALRM,load_round);
	struct itimerval tick;
	memset(&tick,0,sizeof(tick));

	tick.it_value.tv_sec=0;
	tick.it_value.tv_usec=100000;

	tick.it_interval.tv_sec=3;
	tick.it_interval.tv_usec=0;

	res = setitimer(ITIMER_REAL,&tick,NULL);


	// loop main thread, sleep until signal allert
	// exit function when buffer is full, or msg transmit succeed
	while(1)
	{
		if(round_count>MAXROUND)
		{
			break;
		}
		if(end_capture==true)
		{
			break;
		}
		sleep(1);
	}
	printf("Msg:%s\n",msg);
	printf("Loop Turn:%d\n",round_count);
	fprintf(logfile,"*****************\n");
	for(int i=0;i<bit_count;i++)
	{
		fprintf(logfile,"%d.",decoded[i]);
	}
	fprintf(logfile,"*****************\n");
	for(int i=0;i<char_count;i++)
	{
		fprintf(logfile,"%d\n",msg[i]);
	}
	fprintf(logfile,"*****************\n");
	fprintf(logfile,"%s\n",msg);
	
	fclose(logfile);
	fclose(fp);
	
	return 0;
}

void load_round(int sig){
	if( fp == NULL)
	{
		printf("Cannot open file!");
		exit(1);
	}
	if( buff == NULL)
	{
		printf("failed alloc");
		exit(1);
	}
	
	if(round_count>MAXROUND)
	{
		return;
	}
	struct timeval t1,t2;
	double timeuse;
	gettimeofday(&t1,NULL);

	fseek(fp, 0, SEEK_SET);
	fread(buff, sizeof(unsigned char),MAXREAD,fp);
	round_count = round_count + 1;

	gettimeofday(&t2,NULL);
	timeuse = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec)/1000000.0;
	listen[round_count] = timeuse;
	ttimeuse = t1.tv_sec + (t1.tv_usec )/1000000.0;
	// first stage test: check the read time gaps
	printf("[*]time:%f:%f\n",ttimeuse,timeuse);
	fprintf(logfile,"%f\t%f\n",ttimeuse,timeuse);
	// second stage test:exam the value and intime generate the message
	// first can be the simply binary value
	// second be the char buf
	/*
	printf("rcot:%d,prev:%f,pres:%s,begc:%s,endc:%s,pred:%s,rec_ned:%s,prec:%d,bitc:%d,chac:%d\n",
		round_count,
		prevalue,
		(prestatus)?"T":"F",
		(begin_capture)?"T":"F",
		(end_capture)?"T":"F",
		(prefix_done)?"T":"F",
		(record_need)?"T":"F",
		prefix_count,
		bit_count,
		char_count
		);
	*/
	if(begin_listen==false)
	{
		begin_listen=true;
		prevalue = timeuse;
		return;
	}
	if(end_capture==true)
	{
		return;
	}
	if(prestatus==true && begin_capture == false)
	{
		begin_capture = true;
	}

	// check current status
	if((timeuse - prevalue)>gaps)
	{
		prestatus = true;
	}
	else if((prevalue - timeuse)>gaps)
	{
		prestatus = false;
	}
	prevalue = timeuse;
	if(prefix_done == true)
	{
		record_need = (record_need)?false:true; // I forgot how to write the xor
	}

	if(prefix_done==false )
	{
		if(prestatus==true)
		{
			if(prefix_count<2)
			{
				prefix_count+=1;
			}
			else if(prefix_count==2)
			{
				prefix_count+=1;
				prefix_done=true;
				record_need =true;
			}
		}
		return;
	}

	// record the bit if needed (2*3s)
	// bool is not an identical variable
	// abandon prefix
	if(record_need == true)
	{
		decoded[bit_count]=prestatus;
		//printf("prestatus:%s",(prestatus)?"T":"F");
		printf("bit received:");
		for(int i=0;i<=bit_count;i++)
		{
			printf("%d.",decoded[i]);

		}
		printf("\n");
		char ss = 1;
		
		if( (bit_count>0) && (bit_count%8==0))
		{
			for(int i=0;i<8;i++)
			{
				ss = ss<<1;
				if(decoded[bit_count-8+i]==true)
				{
					ss = ss|1;
				}
			}
			
			printf("get:%d\n",ss);
			msg[char_count++]=ss;
			if(ss==0)
			{
				end_capture=true;
			}
			
		}
		bit_count +=1;
	}
	
}


	

