#include<stdio.h>
#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<string.h>
#include <sys/msg.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>

#define TNUM 1024

pthread_t threads[TNUM];
int thread_index = 0;


struct msgbuf {
  long mtype;
  int pid;
  int shmid;
  int thread_index;
  int m1_x;
  int m1_y;
  int m2_x;
  int m2_y;
};

struct thread_param {
  int shmid;
  int client_pid;
  int m1_x;
  int m1_y;
  int m2_x;
  int m2_y;
};

void *thread_func(void *args){
    struct thread_param *arg = args;
    int shmid = arg -> shmid;
    int client_pid = arg -> client_pid;
    int x1=arg ->m1_x;  int y1=arg ->m1_y;  int x2=arg ->m2_x;  int y2=arg ->m2_y;
    int *shm, *s;

    int m1[x1][y1];  /// initilize 2two input matrix and result matrix
    int m2[x2][y2];
    int result_matrix[x1][y2];
    
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) { /// attach shm to thread
        perror("shmat -- error when attacing shared memory");
        exit(1);
    }
    s = shm; //// writing to shared memory
    while( *s == 0){  /// wait for client to set first value of shm to -1 indicating writing memeory is done 
        /// wait for client to write shm
    }
    sleep(1);
    s++;

    for(int i = 0; i<x1;i++)  ////  reading from shm to matrixes
        for(int j = 0; j<y1;j++)
            m1[i][j] = *s++;
    for(int i = 0; i<x2;i++)
        for(int j = 0; j<y2;j++)
            m2[i][j] = *s++;
            
    s = shm;
    s++;
    for(int i = 0; i<x1;i++) ////  calculating result matrix
        for(int j = 0; j<y2;j++){
            result_matrix[i][j] = 0;
            for (int k = 0; k < y1; k++) 
                result_matrix[i][j] += m1[i][k] * m2[k][j];
            *s++ = result_matrix[i][j];  
        }   ////  reading from shm and calculeting
    *shm=0; // set first integer value of shm to 0 so that client will know calculating is done
    shmdt(shmid);
    
    while(*s!=-1) ;    // wait for client to finish read, after that remove shm

    if ((shmctl(shmid, IPC_RMID, NULL)) == -1) {
        perror("shmctl: shmctl failed");
        exit(1);
    }
    else{
        // printf("shm removed succesfully \n");
    }
    free(args);
    // fflush(stdout); /// for debugging
}

int create_shmid(int client_pid,int x1,int y1,int x2, int y2){
    key_t key;
    key = ftok("./myserver.c", client_pid);
    int local_shmid;
    if ((local_shmid = shmget(key, sizeof(int)*(x1*y1+x2*y2+2), IPC_CREAT | 0666)) < 0) {/// creation of shared memory
        perror("shmget -- error when crating shm");
        exit(1);
    }
    return local_shmid;
}

int main(){
    printf("\nserver runnig");
    int msgkey = 1234; /// message que at server side for recieving 
    int qid;
    qid = msgget(msgkey, IPC_CREAT | 0666);

    int msgkey2 = 123235; ///  /// message que at server side for sending 
    int qid2;
    qid2 = msgget(msgkey2, IPC_CREAT | 0666);

    int waiterAtqueue=0;
    
    struct msgbuf msg;
    struct msgbuf msgtosend;
    msgtosend.mtype = 1;

    char c;
    int shmid;
    key_t key;
    char *shm, *s;

    while(1 == 1){
        // printf("\nserver loop\n");
        if (msgrcv(qid, (void *) &msg, 7*sizeof(int), 0,MSG_NOERROR | IPC_NOWAIT) == -1) {
             waiterAtqueue=0;
            if (errno == ENOMSG) {  // no message at quue
            }
            else{
                perror("msgrcv-unexpected error occured");
                exit(EXIT_FAILURE);
            }  
        } else { /// messsage recieved succesfully from client to server
            
            if(msg.shmid == -1){
                pthread_cancel(threads[msg.thread_index]);
            }
        
            else{
            shmid=create_shmid(msg.pid,msg.m1_x,msg.m1_y,msg.m2_x,msg.m2_y); //// create shmid for client and working thread
            waiterAtqueue=1; // some msg recievd from qid and shared memory is
            msgtosend.mtype= msg.pid;
            msgtosend.shmid= shmid;
            msgtosend.thread_index=thread_index;

            struct thread_param *args = malloc(sizeof *args); /// initilize data to give thread
            args->shmid = shmid;
            args->client_pid = msg.pid;
            args->m1_x = msg.m1_x;
            args->m1_y = msg.m1_y;
            args->m2_x = msg.m2_x;
            args->m2_y = msg.m2_y;
            pthread_create(&threads[thread_index++], NULL, thread_func, (void *) args);
            }
        }
        if(waiterAtqueue){   /// send client info of shared memory (shmid)
            if(msgsnd(qid2,(void *) &msgtosend, 7*sizeof(int),IPC_NOWAIT) == -1){
                perror("msgsnd error");
                exit(EXIT_FAILURE);
            }
            else{
                printf("server sent message to client proces with id %d \n",msg.pid);
            }
        }

    }
}