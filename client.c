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
typedef struct message {
    long    mtype;
    int accno;
    int req;
    int amount;
} message;

int msgid,semid,id;

void semop_init(int semindex,int check)
{
	struct sembuf sop;
	sop.sem_num=semindex;
	sop.sem_op=check;
	sop.sem_flg=0;
	semop(semid,&sop,1);
}

void to_exit(int status)
{
	semop_init(id-1,1);
	exit(status);
}

int semop_nowait(int semindex,int check)
{
	struct sembuf sop;
	sop.sem_num=semindex;
	sop.sem_op=check;
	sop.sem_flg=IPC_NOWAIT;
	return semop(semid,&sop,1);
}

int send_msg( int qid, struct message *q_message )
{
    int result, length;

    length = sizeof(struct message) - sizeof(long);

    if((result = msgsnd( qid, q_message, length, 0)) == -1)
    {
        perror("Error in sending message");
		to_exit(1);
    }

    return(result);
}
int read_message( int qid, long type, struct message *q_message )
{
    int result, length;
    length = sizeof(struct message) - sizeof(long);

    if((result = msgrcv( qid, q_message, length, type,  0)) == -1)
    {
        perror("Error in receiving message");
		to_exit(1);
    }

    return(result);
}

int populate_id(int id)
{
	FILE *fp=fopen("ATM_Locator.txt","r");
	int i;
	int x,y,z,w;
	for(i=0;i<ATM_NUM;i++)
	{
		fscanf(fp,"%d %d %d %d",&x,&y,&z,&w);
		if(x==id)
		{
			msgid=msgget((key_t)y,IPC_CREAT|0666);
			semid=semget((key_t)z,ATM_NUM,0666|IPC_CREAT);
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return -1;
}
int main()
{
	char inp[15],next_inp[15];
	int amount;
	int enter=0;
	int pid=getpid();
	signal(SIGINT,to_exit);
	while(1)
	{
		printf("Client: ");
		scanf("%s",inp);
		if(strcmp(inp,"ENTER")==0)
		{
			scanf("%s",next_inp);
			if(enter==1)
			{
				printf("Please LEAVE first\n");
				continue;
			}
			id=atoi(next_inp+3);
			if(populate_id(id)==-1)
			{
				printf("Please Enter valid ATM id\n");
				continue;
			}
			if(semop_nowait(id-1,-1)==-1)
			{
				printf("%s occupied\n",next_inp);
				continue;
			}
			enter=1;
			message msg;
			msg.mtype=(long)id;
			msg.accno=pid;
			msg.req=ENTER;
			msg.amount=-1;
			send_msg(msgid,&msg);
			printf("Welcome Client %d\n",pid);
			printf("Available options :\n");
			printf("ADD <amount>\n");
			printf("WITHDRAW <amount>\n");
			printf("VIEW\n");
			printf("LEAVE\n");
		}
		else if(strcmp(inp,"ADD")==0)
		{
			scanf("%s",next_inp);
			if(enter==0)
			{
				printf("You haven't entered yet\n");
				continue;
			}
			amount=atoi(next_inp);
			if(amount <= 0)
			{
				printf("Please ADD a valid amount\n");
				continue;
			}
			message msg;
			msg.mtype=(long)id;
			msg.accno=pid;
			msg.req=DEPOSIT;
			msg.amount=amount;
			send_msg(msgid,&msg);
		}
		else if(strcmp(inp,"WITHDRAW")==0)
		{
			scanf("%s",next_inp);
			if(enter==0)
			{
				printf("You haven't entered yet\n");
				continue;
			}
			amount=atoi(next_inp);
			if(amount <= 0)
			{
				printf("Please withdraw a valid amount\n");
				continue;
			}
			message msg;
			msg.mtype=(long)id;
			msg.accno=pid;
			msg.req=WITHDRAW;
			msg.amount=amount;
			send_msg(msgid,&msg);
			read_message(msgid,(long)pid,&msg);
			if(msg.amount==1)
			{
				printf("Withdrawal Successful\n");
			}
			else if(msg.amount==-1)
			{
				printf("Balance Insufficient\n");
			}
		}
		else if(strcmp(inp,"LEAVE")==0)
		{
			if(enter==0)
			{
				printf("You haven't entered yet\n");
				continue;
			}
			message msg;
			msg.mtype=(long)id;
			msg.accno=pid;
			msg.req=LEAVE;
			msg.amount=-1;
			send_msg(msgid,&msg);
			semop_init(id-1,1);
			enter=0;
			printf("Thanks for using this ATM, CLIENT %d\n",pid);
		}
		else if(strcmp(inp,"VIEW")==0)
		{
			if(enter==0)
			{
				printf("You haven't entered yet\n");
				continue;
			}
			message msg;
			msg.mtype=(long)id;
			msg.accno=pid;
			msg.req=VIEW;
			msg.amount=amount;
			send_msg(msgid,&msg);
			read_message(msgid,(long)pid,&msg);
			printf("Available Balance: %d\n",msg.amount);
		}
		else
		{
			printf("Invalid Input\n");
		}
	}
	return 0;
}
