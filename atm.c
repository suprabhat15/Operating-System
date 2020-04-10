#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/sem.h> 
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>


#define ENTER 0
#define WITHDRAW 1
#define DEPOSIT 2
#define VIEW 3
#define LEAVE 4 

#define ATM_NUM 5

int shmid,semid,msgid;
struct acc{
	int accno;
	struct timeval t;
	int balance;
};

struct trans{
	int accno;
	struct timeval t;
	int type; 
	int amount;
};

typedef struct message {
    long    mtype;          
    int id;
    int accno;
    int req; 
} message;

typedef struct message2 {
    long    mtype;         
    int accno;
    int req; 
    int amount;
} message2;

void to_exit(int status)
{
	shmctl(shmid,IPC_RMID,0);
    msgctl(msgid,IPC_RMID,NULL);
	exit(status);
}

int send_msg( int qid, struct message *q_message )
{
    int     result, length;

    length = sizeof(struct message) - sizeof(long);       

    if((result = msgsnd( qid, q_message, length, 0)) == -1)
    {
        perror("Error in sending message");
		to_exit(1);
    }
   
    return(result);
}
int read_msg( int qid, long type, struct message *q_message )
{
    int     result, length;

    length = sizeof(struct message) - sizeof(long);        

    if((result = msgrcv( qid, q_message, length, type,  0)) == -1)
    {
        perror("Error in receiving message");
		to_exit(1);
    }
    
    return(result);
}

int send_msg_2( int qid, struct message2 *q_message )
{
    int     result, length;

    length = sizeof(struct message2) - sizeof(long);       

    if((result = msgsnd( qid, q_message, length, 0)) == -1)
    {
        perror("Error in sending message");
		to_exit(1);
    }
   
    return(result);
}
int read_msg_2( int qid, long type, struct message2 *q_message )
{
    int     result, length;

    length = sizeof(struct message2) - sizeof(long);        

    if((result = msgrcv( qid, q_message, length, type,  0)) == -1)
    {
        perror("Error in receiving message");
		to_exit(1);
    }
    
    return(result);
}

int local_cons(int accno)
{
	FILE *fp=fopen("ATM_Locator.txt","r");
	struct timeval t;
	t.tv_sec=0;
	t.tv_usec=0;
	int i,j;
	int money=0;
	for(i=0;i<ATM_NUM;i++)
	{
		int x,y,z,lmemkey;
		fscanf(fp,"%d %d %d %d",&x,&y,&z,&lmemkey);
		int lshmid=shmget((key_t)lmemkey,(100*sizeof(struct acc)+500*sizeof(struct trans)),IPC_CREAT|0666);
		void *ptr;
		struct acc *ptr1;
		struct trans *ptr2;
		ptr=shmat(lshmid,NULL,0);
	    ptr1=(struct acc*)ptr;
	    ptr2=(struct trans*)(ptr+100*sizeof(struct acc));
	    for(j=0;j<100;j++)
	    {
	    	if(ptr1[j].accno==-1)
	    	{
	    		break;
	    	}
	    	else if(ptr1[j].accno==accno)
	    	{
	    		if(timercmp(&t,&(ptr1[j].t),<)!=0)
	    		{
	    			t=ptr1[j].t;
	    			money=ptr1[j].balance;
	    		}
	    		break;
	    	}
	    }
	    int temp=shmdt(ptr);
	    if(temp==-1)
	    {	
	    	printf("%d\n",i+1);
	    	perror("failed to detach memory segment from ATM id");
	    	to_exit(1);
	    }
	}
	fclose(fp);
	fp=fopen("ATM_Locator.txt","r");
	for(i=0;i<ATM_NUM;i++)
	{
		int x,y,z,lmemkey;
		fscanf(fp,"%d %d %d %d",&x,&y,&z,&lmemkey);
		int lshmid=shmget((key_t)lmemkey,(100*sizeof(struct acc)+500*sizeof(struct trans)),IPC_CREAT|0666);
		void *ptr;
		struct acc *ptr1;
		struct trans *ptr2;
		ptr=shmat(lshmid,NULL,0);
	    ptr1=(struct acc*)ptr;
	    ptr2=(struct trans*)(ptr+100*sizeof(struct acc));
	    for(j=0;j<500;j++)
	    {
	    	if(ptr2[j].accno==accno)
	    	{
	    		if(timercmp(&t,&(ptr2[j].t),<)!=0)
	    		{
	    			if(ptr2[j].type==DEPOSIT)
	    				money=money+ptr2[j].amount;
	    			else
	    				money=money-ptr2[j].amount;
	    		}
	    	}
	    }
	    int temp=shmdt(ptr);
	    if(temp==-1)
	    {
	    	printf("%d\n",i+1);
	    	perror("failed to detach memory segment from ATM id");
	    	to_exit(1);
	    }
	}
	fclose(fp);
	return money;
}

int main(int argc,char *argv[])
{
	if(argc<5)
	{
		printf("Usage: <id> <msgqkey> <semkey> <memsegkey>\n");
		exit(1);
	}
    signal(SIGINT,to_exit);
	signal(SIGTERM,to_exit);
    signal(SIGKILL,to_exit);
    int msgkey,semkey,memsegkey,mastermsgid,id,mastermsgkey=1230;
    sscanf(argv[1],"%d",&id);
    sscanf(argv[2],"%d",&msgkey);
    sscanf(argv[3],"%d",&semkey);
    sscanf(argv[4],"%d",&memsegkey);
    void *ptr;
    struct acc *ptr1;
    struct trans *ptr2;
    shmid=shmget((key_t)memsegkey,(100*sizeof(struct acc)+500*sizeof(struct trans)),IPC_CREAT|0666);
	msgid=msgget((key_t)msgkey,IPC_CREAT|0666);
	if(shmid==-1)
    {
    	perror("Failure of Shared memory creation ");
    	exit(1);
    }
    if(msgid==-1)
    {
    	perror("Failure of Message Queue creation");
    	exit(1);
    }
	mastermsgid=msgget((key_t)mastermsgkey,IPC_CREAT|0666);
    if(mastermsgid==-1)
    {
        perror("Message Queue creation failed for master");
        exit(1);
    }
    semid=semget((key_t)semkey,ATM_NUM,0666|IPC_CREAT);
    if(semid==-1)
    {
        perror("Semaphore creation failed");
        exit(1);
    }
    semctl(semid,(id-1),SETVAL,1); //for subsemaphore
    ptr=shmat(shmid,NULL,0);
    ptr1=(struct acc*)ptr;
    ptr2=(struct trans*)(ptr+100*sizeof(struct acc));
    int i;
    for(i=0;i<500;i++)
    {
        ptr2[i].accno=-1;
    }
    for(i=0;i<100;i++)
    {
        ptr1[i].accno=-1;
    }
    while(1)
    {	
    	message2 msg2;
        read_msg_2(msgid,(long)id,&msg2);
    	int accno=msg2.accno;
    	int req=msg2.req;
    	if(req==ENTER)
    	{
    		message msg_init;
	    	msg_init.accno=accno;
	    	msg_init.id=id;
	    	msg_init.req=ENTER;
	    	msg_init.mtype=1230;
    		send_msg(mastermsgid,&msg_init);
            printf("ATM%d: Client %d entered\n",id,accno);
    	}
    	else if(req==WITHDRAW)
    	{
    		printf("ATM%d: running a local consistency check for a/c %d\n",id,accno);
            int money=local_cons(accno);
    		message2 msgtemp; 
    		if(money>=msg2.amount)
    		{
    			msgtemp.mtype=(long)accno;
	    		msgtemp.accno=accno;
	    		msgtemp.req=-1;
	    		msgtemp.amount=1;
    			int flg=0;
	    		for(i=0;i<500;i++)
	    		{
	    			if(ptr2[i].accno==-1)
	    			{
	    				ptr2[i].accno=accno;
	    				gettimeofday(&(ptr2[i].t),NULL);
	    				ptr2[i].type=WITHDRAW;
	    				ptr2[i].amount=msg2.amount;
	    				flg=1;
	    				break;
	    			}
	    		}
	    		if(flg==0)
	    		{
	    			perror("Memory Overflow");
	    			to_exit(1);
	    		}
    		}
    		else
    		{
	    		msgtemp.mtype=(long)accno;
	    		msgtemp.accno=accno;
	    		msgtemp.req=-1;
	    		msgtemp.amount=-1;
    		}
    		send_msg_2(msgid,&msgtemp);
    	}
    	else if(req==DEPOSIT)
    	{
    		int flg=0;
    		for(i=0;i<500;i++)
    		{
    			if(ptr2[i].accno==-1)
    			{
    				ptr2[i].accno=accno;
    				gettimeofday(&(ptr2[i].t),NULL);
    				ptr2[i].type=DEPOSIT;
    				ptr2[i].amount=msg2.amount;
    				flg=1;
    				break;
    			}
    		}
    		if(flg==0)
    		{
    			perror("Memory Overflow");
    			to_exit(1);
    		}
    	}	
    	else if(req==VIEW)
    	{
    		printf("ATM%d: running a consistency check for a/c %d\n",id,accno);
            message msgsend,msgrecv;
    		msgsend.id=id;
    		msgsend.mtype=1230;
    		msgsend.accno=accno;
    		msgsend.req=VIEW;
    		send_msg(mastermsgid,&msgsend);
    		read_msg(mastermsgid,(long)id,&msgrecv);
    		for(i=0;i<100;i++)
    		{
    			if(ptr1[i].accno==accno)
    			{
    				ptr1[i].balance=msgrecv.req;
    				break;
    			}
                else if(ptr1[i].accno==-1)
                {
                    ptr1[i].accno=accno;
                    gettimeofday(&(ptr1[i].t),NULL);
                    ptr1[i].balance=msgrecv.req;
                    break;
                }
    		}
    		message2 msgtemp;
    		msgtemp.mtype=accno;
    		msgtemp.accno=accno;
    		msgtemp.req=-1;
    		msgtemp.amount=ptr1[i].balance;
    		send_msg_2(msgid,&msgtemp);
    	}
        else if(req==LEAVE)
        {
            printf("ATM%d: Client %d left\n",id,accno);
        }
    	else
    	{
    		perror("Invalid Request");
    	}
    }
    return 0;	
}
