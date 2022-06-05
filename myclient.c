#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <sys/msg.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include<errno.h>

struct msgbuf {
  long mtype;
  int pid; /// proces id of client
  int shmid;  //// shared memory id 
  int thread_index;
  int m1_x;
  int m1_y;
  int m2_x;
  int m2_y;
};



int main(int argc, char *argv[]){ //// x denotes row y denotes columns !!!!!!!!!
    char str1[]="Sample string";
    int m1_x; int m1_y; int m2_x; int m2_y;
    if(argc < 3){  
        printf("need two matrix to read.\n");  
        exit(-1);
    } 
    FILE *in_file1 = fopen(argv[1], "r"); // read only 
    FILE *in_file2  = fopen(argv[2], "r"); // read only 
    if (in_file1 == NULL || in_file2 == NULL) 
        {   
            printf("Error! Could not open file\n"); 
            exit(-1); // must include stdlib.h 
        } 
    fscanf(in_file1, "%d %d", &m1_x, &m1_y);
    fscanf(in_file2, "%d %d", &m2_x, &m2_y);

    if(m1_y != m2_x){
        printf("these two matrixes cannot be multiplied\n");  
        exit(-1);
    }
    int m1[m1_x][m1_y];
    int m2[m2_x][m2_y];
   
    for(int i =0; i< m1_x ;i++ )
        for(int j = 0; j<m1_y ;j++)
            fscanf(in_file1, "%d",  &m1[i][j]);
     for(int i =0; i< m2_x ;i++ )
        for(int j = 0; j<m2_y ;j++)
            fscanf(in_file2, "%d",  &m2[i][j]);

    struct msgbuf msg;
    struct msgbuf msgtoget;
    pid_t pid = getpid();
    msg.mtype = 1;
    msg.pid=pid;

    int msgkey = 1234; /// message quee identifer for sending message to server
    int qid;
    qid = msgget(msgkey, 0666);

    int msgkey2 = 123235; /// message quee identifer for recieving messages from server
    int qid2;
    qid2 = msgget(msgkey2, 0666);

    int shmid;
    int *shm, *s;

    msg.m1_x=m1_x; msg.m1_y=m1_y; msg.m2_x=m2_x; msg.m2_y=m2_y;

    printf("client runnig\n");
    
    if(msgsnd(qid,(void *) &msg, 7*sizeof(int), IPC_NOWAIT)==-1){
        perror("msgsnd error-client couldnt send message");
        exit(EXIT_FAILURE);
    }
    else{
            // printf("client sended message\n");
    }

    if (msgrcv(qid2, (void *) &msgtoget, 7*sizeof(int), pid,MSG_NOERROR ) == -1) { /// wait for server response
        if (errno == ENOMSG) {
            // no message at quue
        }
        else{
            perror("msgrcv-unexpected error occured");
            exit(EXIT_FAILURE);
        }
    }
    else{
        if ((shm = shmat(msgtoget.shmid, NULL, 0)) == (char *) -1) {
            perror("shmat -- error when attacing shared memory\n");
            exit(1);
        }
        s = shm; //// writing to shared memory
        s++;
    
        for(int i = 0; i<m1_x;i++) // writing to shm
            for(int j = 0; j<m1_y;j++)
                *s++ = m1[i][j];
        for(int i = 0; i<m2_x;i++)
            for(int j = 0; j<m2_y;j++)
                *s++ = m2[i][j];

        *s=-1; /// belki buraya sil
        *shm=-1;  /// set first valkue of shm for server to process
            while( *shm == -1){ /// first value of shared memort is for both result control 
            //// wait for server thread to calculatee
        }
        s = shm;
        s++;
        printf(" result matrix is \n ");
        for(int i =0; i< m1_x ;i++ ){
            for(int j = 0; j<m2_y ;j++)
                printf(" %d ",*s++);
            printf(" \n ");
        }
        shmdt(shmid); 
        msg.thread_index = msgtoget.thread_index;
        msg.shmid=-1;
        if(msgsnd(qid,(void *) &msg, 7*sizeof(int), IPC_NOWAIT)==-1){ /// sender notify server for   for terminating thread with index which server send earlier with smhid
            perror("msgsnd error-client couldnt send message");
            exit(EXIT_FAILURE);
        }
        *s=-1; /// reading is finished
         
    
    }

    
}