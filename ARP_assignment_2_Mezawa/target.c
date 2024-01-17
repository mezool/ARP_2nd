#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <unistd.h> 
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <sys/mman.h>
#include "include/constants.h"

int main(int argc, char *argv[]) 
//Declarations about Variables and Pipes
{
    int send_pipe_read;
    int send_pipe;
    int receive_pipe;
    int receive_pipe_write;

    char ask_char[MSG_LEN]; 
    ask_char[0] = ASK_CHAR;
    char askread_char[MSG_LEN];
    askread_char[0] = ASKR_CHAR;
    char read_str[MSG_LEN];
    char format_string[MSG_LEN] = "%d";
    char ack_char = ACK_CHAR;
    char exp_char = EXCEPTION_CHAR;
    char send_str[MSG_LEN];
    int send_data[MAX_DATA] = {0,0,0,0,0,0,0,0,0,0};
    int read_data[5];
    int writer_num = -1;
    int i;
    int gc = 0;
    int timescore[5] = {0.1, 0.2, 0.3, 0.4, 0.5};
    double score[2] = {0,-100};
//Get descriptors from command line arguments
    if(argc == 3){
        sscanf(argv[1],"%i", &writer_num);
        sscanf(argv[2], "%d %d %d %d", &send_pipe_read, &send_pipe, &receive_pipe, &receive_pipe_write);
    } else {
        printf("wrong number of arguments\n"); 
        return -1;
    }

    int in_progress = 0;
//Close unnecessary pipes
    close(send_pipe_read);
    close(receive_pipe_write);

    printf("Writer number %i \n", writer_num);
    printf("Sending on pipe %i \n", send_pipe);
    printf("Receiving on pipe %i \n", receive_pipe);
    sleep(1);

    srandom( time(NULL) + writer_num * SEED_MULTIPLIER);

    // initialize random values
    for (int i = 0; i < 5; i++)
    {
        send_data[2 * i] = random() % MAX_NUMBER1; // position of object (x)
        send_data[2 * i + 1] = random() % (MAX_NUMBER2 - 3); // position of object (y)
    }

    sprintf(send_str, "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i", send_data[0], send_data[1], send_data[2], send_data[3], send_data[4], send_data[5], send_data[6], send_data[7], send_data[8], send_data[9]);
    //printf("Acknowledged, target sent data %i, %i, %i, %i, %i, %i, %i, %i, %i, %i \n", send_data[0], send_data[1], send_data[2], send_data[3], send_data[4], send_data[5], send_data[6], send_data[7], send_data[8], send_data[9]);
    write(send_pipe, send_str, strlen(send_str) + 1);

    //WD
    //Publish your pid
    pid_t watchdog_pid;
    pid_t my_pid = getpid();

    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    FILE *pid_fp = fopen(fnames[writer_num], "w");
    if (pid_fp == NULL) {
        perror("fopen");
       return -1;
    }
    fprintf(pid_fp, "%d", my_pid);
    fclose(pid_fp);

    //printf("Published pid %d \n", my_pid);
    // Read watchdog pid
    FILE *watchdog_fp = NULL;
    struct stat sbuf;

    /* call stat, fill stat buffer, validate success */
    if (stat (PID_FILE_PW, &sbuf) == -1) {
        perror ("error-stat");
        return -1;
    }
    // waits until the file has data
    while (sbuf.st_size <= 0) {
        if (stat (PID_FILE_PW, &sbuf) == -1) {
            perror ("error-stat");
            return -1;
        }
        usleep(50000);
    }

    watchdog_fp = fopen(PID_FILE_PW, "r");

    fscanf(watchdog_fp, "%d", &watchdog_pid);
    //printf("watchdog pid %d \n", watchdog_pid);
    fclose(watchdog_fp);

    clock_t start_time, end_time;
    double elapsed_time;
    start_time = clock();

    while (1) 
    {   
            // process to read the drone's position
            // send ask
            //printf("now target is in mode 1\n");
            if(in_progress == 0){
                //printf("Target requested new information\n");
                write(send_pipe, askread_char, strlen(askread_char) + 1);
                in_progress = 1;
            } else {
                //printf("Try to read \n");
                read(receive_pipe, read_str, MSG_LEN);
                //printf("Read data: %s\n", read_str);
                if (read_str[0] == REJ_CHAR){ 
                    printf("Drone hasn't arrived \n");
                } else if (read_str[0] == EXCEPTION_CHAR){
                    printf("Target has nothing to read \n");

                } else { // receive new position
                    sscanf(read_str, format_string, &read_data);
                    //printf("Drone arrived one of the targets \n");
                    for (i = 0; i < 5; ++i) {
                        printf("%d ", read_data[i]);
                    }
                    printf("\n");
                    end_time = clock();
                    elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
                    printf("Elapsed time: %f seconds\n", elapsed_time);
                    score[0] = score[0] + 1000*(timescore[gc] - elapsed_time);
                    sprintf(send_str, "%f, %f", score[0], score[1]);
                    // printf("Acknowledged, target sent score %f\n", score[0]);
                    write(send_pipe, send_str, strlen(send_str) + 1);
                    //printf("Target sent write request\n");
                    gc = gc + 1;
                }
                in_progress = 0;
                // mode = 0;
            }
            usleep(100000);
        // }        
        if(kill(watchdog_pid, SIGUSR1) < 0)
        {
            perror("kill");
        }
        //printf("target sent signal to watchdog\n"); // added to debug
    } 
    return 0; 
} 