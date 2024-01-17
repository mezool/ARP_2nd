//from main.c from homework_5
#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "include/constants.h"

//Interprocess communication by creating several child processes, each executing a different program
//Execute each process
int main(int argc, char *argv[])
{
    // Pids for all children
    pid_t child_writer0;
    pid_t child_writer1;
    pid_t child_target;
    pid_t child_reader0;
    pid_t child_reader1;
    pid_t child_server;
    pid_t child_watchdog;

    //Pipes for inter-process communication
    int writer0_server[2];
    int server_writer0[2];
    int writer1_server[2];
    int server_writer1[2];
    int target_server[2];
    int server_target[2];
    int reader0_server[2];
    int server_reader0[2];
    int reader1_server[2];
    int server_reader1[2];
// make pipes
    pipe(writer0_server);
    pipe(server_writer0);
    pipe(writer1_server);
    pipe(server_writer1);
    pipe(target_server);
    pipe(server_target);
    pipe(reader0_server);
    pipe(server_reader0);
    pipe(reader1_server);  
    pipe(server_reader1);

         // Make a log file with the start time/date
    time_t now = time(NULL);
    struct tm *timenow;
    timenow = gmtime(&now);

    char logfile_name[80];

    //There should be a check that the log folder exists but I haven't done that
    sprintf(logfile_name, "./log/watchdog%i-%i-%i_%i:%i:%i.txt", timenow->tm_year + 1900, timenow->tm_mon +1, timenow->tm_mday, timenow->tm_hour +1, timenow->tm_min, timenow->tm_sec);

    fopen(PID_FILE_PW, "w");
    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    for(int i = 0; i < NUM_PROCESSES; i++)
    {
        fopen(fnames[i], "w");
    }

    int res;
    int num_children;

    char writer0_args[80];
    char writer1_args[80];
    char target_args[80];
    char reader0_args[80];
    char reader1_args[80];
// get pipe discriptors
    sprintf(writer0_args, "%d %d %d %d", writer0_server[0], writer0_server[1], server_writer0[0], server_writer0[1]);
    sprintf(writer1_args, "%d %d %d %d", writer1_server[0], writer1_server[1], server_writer1[0], server_writer1[1]);
    sprintf(target_args, "%d %d %d %d", target_server[0], target_server[1], server_target[0], server_target[1]);
    sprintf(reader0_args, "%d %d %d %d", reader0_server[0], reader0_server[1], server_reader0[0], server_reader0[1]);
    sprintf(reader1_args, "%d %d %d %d", reader1_server[0], reader1_server[1], server_reader1[0], server_reader1[1]);
//Use fork to create child processes, each executing a different program.
//execvp is used to execute each child process.
    child_server = fork(); //-1 error　0 fork as child　+ fork and return to parent
    if (child_server < 0) {
        perror("Fork");
        return -1;
    }

    if (child_server == 0) {
        char * arg_list[] = { "konsole", "-e", "./serverp","0", logfile_name, writer0_args, writer1_args, target_args, NULL };
        printf("Executing command: konsole -e ./serverp 0 %s %s %s %s\n", logfile_name, writer0_args, writer1_args, target_args);
        execvp("konsole", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;

//drone
    child_writer0 = fork();
    if (child_writer0 < 0) {
        perror("Fork");
        return -1;
    }

    if (child_writer0 == 0) {
        char * arg_list[] = {"./dronep", "1", writer0_args, NULL };
        execvp("./dronep", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;

//object
    child_writer1 = fork();
    if (child_writer1 < 0) {
        perror("Fork");
        return -1;
    }

    if (child_writer1 == 0) {
        char * arg_list[] = {"./object", "2", writer1_args, NULL };
        execvp("./object", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;

//target
    child_target = fork();
    if (child_target < 0) {
        perror("Fork");
        return -1;
    }

    if (child_target == 0) {
        char * arg_list[] = {"./target", "3", target_args, NULL };
        execvp("./target", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;

    // create keyboard manager
    child_reader0 = fork();
    if (child_reader0 < 0) {
        perror("Fork");
        return -1;
    }

if (child_reader0 == 0) {
        char * arg_list[] = { "konsole", "-e", "./keyboard", NULL };
        execvp("konsole", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;

    sleep(3);

    // Create watchdog
    child_watchdog = fork();
    if (child_watchdog < 0) {
        perror("Fork");
        return -1;
    }

    if (child_watchdog == 0) {
        char * arg_list[] = {"./watchdog", NULL};
        execvp("./watchdog", arg_list);
        perror("execvp");
	return 0;
    }
    num_children += 1;
   
    
    //wait for all children to terminate
    for(int i = 0; i < num_children; i ++){
        wait(&res);
    }

    return 0;
}
