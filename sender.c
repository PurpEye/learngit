#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#define MAXLEN 1048576*260
#define MAXREAD 1048576*260

// function define
void load_round(int sig);

// IO variable
int readpos = 0;
FILE *fp=NULL;
unsigned char *buff=NULL;

// Time variable
struct timeval tt1,tt2;
double ttimeuse;

// Msg transform
bool *orders = NULL;
int round_count = 0;
int tsize = 0;
int msize = 0;


void gen_orders(char *str)
{
	int change,k=0,mask=128;
	// prefix
	for (int i=0; i<1;i++)
	{
		orders[i*5]=true;
		orders[i*5+1]=false;
		orders[i*5+2]=true;
		orders[i*5+3]=false;
		orders[i*5+4]=true;
		k = k+5;
	}
	for (int i=0; i<tsize;i++)
	{
		for(int j=0;j<8;j++)
		{
			orders[k++]=(mask&(str[i]))?true:false;
			mask>>=1;
		}
		mask=128;
	}
	msize=k;
	return;
}

int main(int argc, char *argv[])
{

	if(argc<3)
	{
		printf("input file name");
		exit(1);
	}
	if( (fp=fopen(argv[1],"rb")) == NULL)
	{
		printf("Cannot open file!");
		exit(1);
	}
	tsize = strlen(argv[2]);
	printf("Msg to be sent:%s:%d\n",argv[2],tsize);
	orders = (bool*)malloc((16+tsize*24)*sizeof(bool));
	if(!orders)
	{
		printf("failed alloc orders");
		exit(1);
	}
	gen_orders(argv[2]);

	printf("Total size of bytes:%d",msize);
	printf("Binary List:");
	for(int i=0;i<msize;i++)
	{
		printf("%s.",(orders[i]==true)?"1":"0");
	}

	printf("\n");

	gettimeofday(&tt1,NULL);
	srand((unsigned)time(NULL));
	readpos = rand()%100;
	int res = 0;
	signal(SIGALRM,load_round);
	struct itimerval tick;
	memset(&tick,0,sizeof(tick));

	tick.it_value.tv_sec=1;
	tick.it_value.tv_usec=0;

	tick.it_interval.tv_sec=6;
	tick.it_interval.tv_usec=0;

	res = setitimer(ITIMER_REAL,&tick,NULL);

	srand((unsigned)time(NULL));
	int readpos = 0;

	while(1)
	{
		if(round_count>msize+1)
		{
			printf("[*]transaction finished");
			return 0;
		}
		usleep(2000000);
	}

	fclose(fp);
	
	return 0;
}

void load_round(int sig){
	if( fp == NULL)
	{
		printf("Cannot open file!");
		exit(1);
	}
	if(buff)
	{
		free(buff);
	}
	printf("Now allocate\n");
	buff = (unsigned char*)malloc(MAXLEN*sizeof(unsigned char));
	if( buff == NULL)
	{
		printf("failed alloc");
		exit(1);
	}
	
	struct timeval t1,t2;
	double timeuse;
	gettimeofday(&t1,NULL);
	readpos = (readpos+7)%15;
	printf("round:%d,msize:%d,order:%s\n",round_count,msize,((orders[round_count]==true)?"1":"0"));
	if(round_count< msize && orders[round_count]==true)
	{
		fseek(fp, MAXREAD*readpos, SEEK_SET);	
		fread(buff, sizeof(unsigned char),MAXREAD,fp);
	}
	round_count = round_count + 1;

	gettimeofday(&t2,NULL);
	timeuse = t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec)/1000000.0;
	ttimeuse = t1.tv_sec  + (t1.tv_usec )/1000000.0;
	printf("[*]%d/100 time:%f:%f\n",readpos,ttimeuse,timeuse);

}
