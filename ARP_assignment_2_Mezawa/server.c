// server
#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <unistd.h> 
#include <stdlib.h>
#include "include/constants.h"
#include <signal.h>
#include <time.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <ncurses.h>
#include <math.h>

int main(int argc, char *argv[]) 
{
    //placeholders for pipes to close
    int send_pipe_read[4];
    int receive_pipe_write[4];

    //pipes
    int send_pipe_writer_0;
    int receive_pipe_writer_0;
    int send_pipe_writer_1;
    int receive_pipe_writer_1;
    int send_pipe_target_0;
    int receive_pipe_target_0; 
   
    fd_set reading;
    char read_str[MSG_LEN];

    char ack_char[MSG_LEN]; 
    ack_char[0] = ACK_CHAR;

    char rej_char[MSG_LEN];
    rej_char[0] = REJ_CHAR;

    char exp_char[MSG_LEN];
    exp_char[0] = EXCEPTION_CHAR;

    int cell[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
    int cell_t[10] = {0,0,0,0,0,0,0,0,0,0};
    int tmp_cell[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
    int tmp_cell_t[10] = {0,0,0,0,0,0,0,0,0,0};
    int has_changed = 0;

    char format_string[MSG_LEN]="%d";
    char output_string[MSG_LEN];

    char logfile_name[80];

    int counter;
    int F_x = 0;
    int F_y = 0;
    double F_ox[5] = {0,0,0,0,0};
    double F_oy[5] = {0,0,0,0,0};
    double F_tx[5] = {0,0,0,0,0};
    double F_ty[5] = {0,0,0,0,0};
    double tar_pos[10];
    int x_o = 0;
    int y_o = 0;
    int x_o2 = 0;
    int y_o2 = 0;
    int x_o3 = 0;
    int y_o3 = 0;
    int x_o4 = 0;
    int y_o4 = 0;
    int x_o5 = 0;
    int y_o5 = 0;
    int x_t = 0;
    int y_t = 0;
    int x_t2 = 0;
    int y_t2 = 0;
    int x_t3 = 0;
    int y_t3 = 0;
    int x_t4 = 0;
    int y_t4 = 0;
    int x_t5 = 0;
    int y_t5 = 0;
    int M = 1;
    int K = 1;
    double T = 0.1;
    double dist;
    double distx = 6;
    double disty = 6;
    int goal[5] = {1,1,1,1,1};
    double score;
    char tmp_char;
    char targetflag = 1;

    //placeholders for pipes to close
    int send_pipe_writer_0_fds[2];
    int receive_pipe_writer_0_fds[2];
    int send_pipe_writer_1_fds[2];
    int receive_pipe_writer_1_fds[2];
    int send_pipe_target_0_fds[2];
    int receive_pipe_target_0_fds[2];

    // Pipe creation and error checking
    if (pipe(send_pipe_writer_0_fds) == -1 || pipe(receive_pipe_writer_0_fds) == -1 ||
        pipe(send_pipe_writer_1_fds) == -1 || pipe(receive_pipe_writer_1_fds) == -1 ||
        pipe(send_pipe_target_0_fds) == -1 || pipe(receive_pipe_target_0_fds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Assignment of file descriptors
    send_pipe_writer_0 = send_pipe_writer_0_fds[1];
    receive_pipe_writer_0 = receive_pipe_writer_0_fds[0];
    send_pipe_writer_1 = send_pipe_writer_1_fds[1];
    receive_pipe_writer_1 = receive_pipe_writer_1_fds[0];
    send_pipe_target_0 = send_pipe_target_0_fds[1];
    receive_pipe_target_0 = receive_pipe_target_0_fds[0];

    //The program receives four command line arguments and sets the descriptors for each pipe.
    if(argc == 6){
        sscanf(argv[3], "%d %d %d %d", &receive_pipe_writer_0, &receive_pipe_write[0], &send_pipe_read[0], &send_pipe_writer_0);
        sscanf(argv[4], "%d %d %d %d", &receive_pipe_writer_1, &receive_pipe_write[1], &send_pipe_read[1], &send_pipe_writer_1);
        sscanf(argv[5], "%d %d %d %d", &receive_pipe_target_0, &receive_pipe_write[2], &send_pipe_read[2], &send_pipe_target_0);
    } else {
        printf("wrong args %i \n", argc); 
        sleep(15);
        return -1;
    }

    // Extract log file name
    if(argc == 6){
        snprintf(logfile_name, 80, "%s", argv[2]);
    } else {
        printf("wrong args\n"); 
        return -1;
    }
    int process_num;
    if(argc == 6){
        sscanf(argv[1],"%d", &process_num);  
    } else {
        printf("wrong args\n"); 
        return -1;
    }
    //close innecessary pipes
    for(int i = 0; i < 4; i++){
        close(send_pipe_read[i]);
        close(receive_pipe_write[i]);
    }

    
//Compose fd from received information
    int fds[3];
    fds[0] = receive_pipe_writer_0;
    fds[1] = receive_pipe_writer_1;
    fds[2] = receive_pipe_target_0;

    printf("Receiving on %d, %d, %d \n", receive_pipe_writer_0, receive_pipe_writer_1, receive_pipe_target_0);
    printf("Sending on %d, %d, %d \n", send_pipe_writer_0, send_pipe_writer_1, send_pipe_target_0);


    int max_fd = -1;
    for(int i = 0; i < 3; i++){
        if(fds[i] > max_fd){
            max_fd = fds[i];
        }
    }
//Identify maximum
    printf("Max fd %i\n", max_fd);

    pid_t watchdog_pid;
    // Publish your pid
    pid_t my_pid = getpid();

    char *fnames[NUM_PROCESSES] = PID_FILE_SP;

    FILE *pid_fp = fopen(fnames[process_num], "w");
    if (pid_fp == NULL) {
        perror("fopen");
       return -1;
    }
    fprintf(pid_fp, "%d", my_pid);
    fclose(pid_fp);
    //printf("Published pid %d \n", my_pid);

    //WD
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
    printf("watchdog pid %d \n", watchdog_pid);
    fclose(watchdog_fp);

    //Read how long to sleep process for
    // int sleep_durations[3] = PROCESS_SLEEPS_US;
    // int sleep_duration = sleep_durations[process_num];

    int shared_seg_size = (1 * sizeof(cell));
    // initializes/clears contents of logfile
    FILE *lf_fp = fopen(logfile_name, "w");
    if (pid_fp == NULL) {
        perror("fopen");
        return -1;
    }
    fprintf(lf_fp, "Fx, Fy, x, y, score \n");
    fclose(lf_fp);

    
    initscr(); //initialize ncurses
    start_color(); // activate the usage of colors
    init_pair(1, COLOR_BLUE, COLOR_BLACK); // red text and black background
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    noecho(); //Do not display when a key is entered
    curs_set(0);//Do not display the cursor
    timeout(0); // Unblocked in 0 milisecond
    
    
    int row, col;
    getmaxyx(stdscr, row, col);  // Get the number of rows and columns on the screen
    double x_i = MAX_NUMBER1/2;
    double x_i1 = MAX_NUMBER1/2;
    double x_i2 = MAX_NUMBER1/2;
    double y_i = MAX_NUMBER2/2;
    double y_i1 = MAX_NUMBER2/2;
    double y_i2 = MAX_NUMBER2/2;

    while (1) 
    {    
        //reset reading set
        // Select to see if there is readable data
        //select can monitor multiple file descriptors simultaneously
        FD_ZERO(&reading);
        FD_SET(receive_pipe_writer_0, &reading);
        FD_SET(receive_pipe_writer_1, &reading);
        FD_SET(receive_pipe_target_0, &reading);
        int ret_val = select(max_fd + 1, &reading, NULL, NULL, NULL);

        for(int i = 0; i < max_fd + 1; i++){
            if(FD_ISSET(i, &reading)){
                //deal with data from drone
                if(i == receive_pipe_writer_0)
                {
                    read(i, read_str, MSG_LEN);
                // deal with ASK_CHAR msg 
                    if (read_str[0] == ASK_CHAR){
                        // printf("writer 0 wants to write\n");
                        // printf("Writer 0 can write\n");
                        write(send_pipe_writer_0, ack_char, strlen(ack_char) + 1); 
                // deal with the others msg
                    } else {
                        sscanf(read_str, "%i, %i", &tmp_cell[0], &tmp_cell[1]);
                        // printf("Writer 0 set cell 0 to %i, %i\n", tmp_cell[0],tmp_cell[1]);
                    }
                }
                else if (i == receive_pipe_writer_1)
                // deal with data from obstacle
                {
                    read(i, read_str, MSG_LEN);

                    if (read_str[0] == ASK_CHAR){
                        printf("obstacle wants to write\n");
                        //printf("Writer 1 can write\n");
                        write(send_pipe_writer_1, ack_char, strlen(ack_char) + 1);                        
                    } else {
                        sscanf(read_str, "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i", &tmp_cell[2], &tmp_cell[3], &tmp_cell[4], &tmp_cell[5], &tmp_cell[6], &tmp_cell[7], &tmp_cell[8], &tmp_cell[9],&tmp_cell[10], &tmp_cell[11]);
                        printf("Writer 1 set cell 1 to %i, %i, %i, %i, %i, %i, %i, %i, %i, %i \n", tmp_cell[2], tmp_cell[3], tmp_cell[4], tmp_cell[5], tmp_cell[6], tmp_cell[7], tmp_cell[8], tmp_cell[9], tmp_cell[10], tmp_cell[11]);
                    }
                }
                else if (i == receive_pipe_target_0)
                //deal with data from target
                {
                    read(i, read_str, MSG_LEN);

                    if (read_str[0] == ASK_CHAR){
                        printf("target wants to write\n");
                        //printf("Writer 1 can write\n");
                        write(send_pipe_target_0, ack_char, strlen(ack_char) + 1);
                    } else if (read_str[0] == ASKR_CHAR){
                        printf("target wants to read\n");
                        if(has_changed == 1){
                            sprintf(output_string, "%d, %d, %d, %d, %d",
                                    goal[0], goal[1], goal[2], goal[3], goal[4]);
                            printf("Sent %s to target \n", output_string);
                            write(send_pipe_target_0, output_string, strlen(output_string) + 1);
                            has_changed = 0;                        
                        } else {
                            printf("Target cannot read\n");
                            write(send_pipe_target_0, rej_char, strlen(rej_char) + 1); 
                        }
                    // } else if (read_str[1] < 0){
                    //     printf("target sent score");
                    //     sscanf(read_str, "%c %lf", &tmp_char, &score);
                    } else if (targetflag == 1){
                        sscanf(read_str, "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i", &tmp_cell_t[0], &tmp_cell_t[1],&tmp_cell_t[2], &tmp_cell_t[3], &tmp_cell_t[4], &tmp_cell_t[5], &tmp_cell_t[6], &tmp_cell_t[7], &tmp_cell_t[8], &tmp_cell_t[9]);
                        printf("Writer 1 set cell 1 to %i, %i, %i, %i, %i, %i, %i, %i, %i, %i \n", tmp_cell_t[0], tmp_cell_t[1], tmp_cell_t[2], tmp_cell_t[3], tmp_cell_t[4], tmp_cell_t[5], tmp_cell_t[6], tmp_cell_t[7], tmp_cell_t[8], tmp_cell_t[9]);
                        x_t = tmp_cell_t[0];       
                        y_t = tmp_cell_t[1];
                        x_t2 = tmp_cell_t[2];
                        y_t2 = tmp_cell_t[3];
                        x_t3 = tmp_cell_t[4];
                        y_t3 = tmp_cell_t[5];
                        x_t4 = tmp_cell_t[6];
                        y_t4 = tmp_cell_t[7];
                        x_t5 = tmp_cell_t[8];
                        y_t5 = tmp_cell_t[9];
                        write(send_pipe_target_0, exp_char, strlen(exp_char) + 1); 
                        targetflag = 0;
                    } else {
                        printf("target sent score");
                        sscanf(read_str, "%c %lf", &tmp_char, &score);
                        write(send_pipe_target_0, exp_char, strlen(exp_char) + 1); 
                    }
                }
            }
        }
        // renew the data of position
        for (int i = 0; i < 12; ++i){
            cell[i] = tmp_cell[i];
        }
      
        tar_pos[0] = x_t;
        tar_pos[1] = y_t;
        tar_pos[2] = x_t2;
        tar_pos[3] = y_t2;
        tar_pos[4] = x_t3;
        tar_pos[5] = y_t3;
        tar_pos[6] = x_t4;
        tar_pos[7] = y_t4;
        tar_pos[8] = x_t5;
        tar_pos[9] = y_t5;

        // Reads data from shared memory and copies it to the array cells.
        F_x = cell[0];
        F_y = cell[1];
        x_o = cell[2];       
        y_o = cell[3];
        x_o2 = cell[4];
        y_o2 = cell[5];
        x_o3 = cell[6];
        y_o3 = cell[7];
        x_o4 = cell[8];
        y_o4 = cell[9];
        x_o5 = cell[10];
        y_o5 = cell[11];
        double F_oX = 0;
        double F_oY = 0;
        double F_tX = 0;
        double F_tY = 0;

        // calculate repulsive forces
        for(int i = 1; i < 6; i++){
            distx = x_i - cell[2*i];
            disty = y_i - cell[2*i+1];
            dist = distx * distx + disty * disty;
            if ( dist <= 25){
                if (distx < 0.5 && distx > 0){
                    distx = 0.5;
                }
                if (distx > -0.5 && distx < 0){
                    distx = -0.5;
                }
                if (distx == 0){
                    distx = 100;
                }
                if (disty < 0.5 && disty > 0){
                    disty = 0.5;
                }
                if (disty > -0.5 && disty < 0){
                    disty = -0.5;
                }
                if (disty == 0){
                    disty = 100;
                }
                F_ox[i] = 10*(fabs(1/distx) - (1/5))* (1/distx) * (1/fabs(distx));
                F_oy[i] = 10*(fabs(1/disty) - (1/5))* (1/disty) * (1/fabs(disty));
            } else {
                F_ox[i] = 0;
                F_oy[i] = 0;
            }
        }

// calculate of atractive force (same as textbook)
        // for(int i = 0 ; i < 5; i++){
        //     distx = x_i - tar_pos[2*i];
        //     disty = y_i - tar_pos[2*i + 1];
        //     if (distx < 0) {
        //         F_tx[i] = -1 * att;
        //     } else if (tar_pos[2*i] == 500){
        //         F_tx[i] = 0;  
        //     } else {
        //         F_tx[i] = att;
        //     }
        //     if (disty < 0) {
        //         F_ty[i] = -1 * att;
        //     } else if (tar_pos[2*i + 1] == 500){
        //         F_ty[i] = 0;  
        //     } else {
        //         F_ty[i] = att;
        //     }
        // }

// calculate the atractive force
        for(int i = 0; i < 5; i++){
            distx = x_i - tar_pos[2*i];
            disty = y_i - tar_pos[2*i+1];
            dist = distx * distx + disty * disty;
            if ( dist <= 25){
                if (dist > 0.25){
                    double force_magnitude = -10 * (fabs(1/dist) - (1/5)) * (1/dist);
                    // calculate the direction
                    double angle = atan2(disty, distx);
                    // calculate the magnitude of x and y
                    F_tx[i] = force_magnitude * cos(angle);
                    F_ty[i] = force_magnitude * sin(angle);
                } else {
                    F_tx[i] = 0;
                }
            } else {
                F_tx[i] = 0;
                F_ty[i] = 0;
            }
        }
        
        // sum up the forces
        int lengthx = sizeof(F_ox) / sizeof(F_ox[0]);
    
        for (int i = 0; i < lengthx; i++) {
            F_oX += F_ox[i];
        }

        int lengthy = sizeof(F_oy) / sizeof(F_oy[0]);
    
        for (int i = 0; i < lengthy; i++) {
            F_oY += F_oy[i];
        }

        int lentarx = sizeof(F_tx) / sizeof(F_tx[0]);
    
        for (int i = 0; i < lentarx; i++) {
            F_tX += F_tx[i];
        }

        int lentary = sizeof(F_ty) / sizeof(F_ty[0]);
    
        for (int i = 0; i < lentary; i++) {
            F_tY += F_ty[i];
        }

// calculate the drone's position
        x_i2 = x_i1;
        x_i1 = x_i;
        x_i = ( (F_x+F_oX+F_tX)*T*T +M*(2*x_i1-x_i2)+K*T*x_i1) / (M+K*T);
        //x_i = ( F_x*T*T +M*(2*x_i1-x_i2)+K*T*x_i1) / (M+K*T);
        if (x_i > MAX_NUMBER1-1){
            x_i = MAX_NUMBER1-1;
        } else if (x_i < 0){
            x_i = 0;
        }
        y_i2 = y_i1;
        y_i1 = y_i;
        y_i = ( (F_y+F_oY+F_tY)*T*T +M*(2*y_i1-y_i2)+K*T*y_i1) / (M+K*T);
        //y_i = ( F_y*T*T +M*(2*y_i1-y_i2)+K*T*y_i1) / (M+K*T);
        if (y_i > MAX_NUMBER2-4){
            y_i = MAX_NUMBER2-4;
        } else if (y_i < 0){
            y_i = 0;
        }

// move the target when drone arrive the target position
        if (fabs(x_i - x_t) < 1 && fabs(y_i - y_t) < 1 ){
            goal[0] = 0;
            has_changed = 1;
            x_t = 500;
            y_t = 500;
        }
        if (fabs(x_i - x_t2) < 0.5 && fabs(y_i - y_t2) < 1 && goal[0] == 0 ){
            goal[1] = 0;
            has_changed = 1;
            x_t2 = 500;
            y_t2 = 500;
        }
        if (fabs(x_i - x_t3) < 0.5 && fabs(y_i - y_t3) < 1 && goal[1] == 0 ){
            goal[2] = 0;
            has_changed = 1;
            x_t3 = 500;
            y_t3 = 500;
        }
        if (fabs(x_i - x_t4) < 0.5 && fabs(y_i - y_t4) < 1 && goal[2] == 0 ){
            goal[3] = 0;
            has_changed = 1;
            x_t4 = 500;
            y_t4 = 500;
        }
        if (fabs(x_i - x_t5) < 0.5 && fabs(y_i - y_t5) < 1 && goal[3] == 0 ){
            goal[4] = 0;
            has_changed = 1;
            x_t5 = 500;
            y_t5 = 500;
        }

// ncurses
        erase();   // clear the window
        int ch = getch(); //wait for key input
        if (ch == 'q') break; // terminate if "q" is input
        attron(COLOR_PAIR(1)); // activate the pair of colors
        mvprintw(y_i, x_i, "+"); // show drone
        attroff(COLOR_PAIR(1)); // void the pair
        attron(COLOR_PAIR(2)); // activate the pair of colors
        mvprintw(y_o, x_o, "o"); // show object
        mvprintw(y_o2, x_o2, "o"); // show object
        mvprintw(y_o3, x_o3, "o"); // show object
        mvprintw(y_o4, x_o4, "o"); // show object
        mvprintw(y_o5, x_o5, "o"); // show object
        attroff(COLOR_PAIR(2)); // void the pair
        attron(COLOR_PAIR(3)); // activate the pair of colors
        mvprintw(y_t, x_t, "1"); // show object
        mvprintw(y_t2, x_t2, "2"); // show object
        mvprintw(y_t3, x_t3, "3"); // show object
        mvprintw(y_t4, x_t4, "4"); // show object
        mvprintw(y_t5, x_t5, "5"); // show object
        attroff(COLOR_PAIR(3)); // void the pair
        double F_xall =  F_x+F_oX+F_tX;
        double F_yall =  F_y+F_oY+F_tY;
        mvprintw(MAX_NUMBER2 -2, 2, "Fx = %d , Fy = %d , x = %f , y = %f, F_extX = %f, F_extY = %f, score = %f", F_x, F_y, x_i, y_i, F_oX+F_tX, F_oY+F_tY, score);
        WINDOW *win = newwin(MAX_NUMBER2-3, MAX_NUMBER1, 0, 0);  // make main window
        box(win, 0, 0);  // drow the lines
        wrefresh(win);
        WINDOW *iwin = newwin(3, MAX_NUMBER1, MAX_NUMBER2-3, 0);  // make inspection window
        box(iwin, 0, 0);  // drow the lines
        wrefresh(iwin);
        refresh(); // renew the screen
        //printf("x_i is: %f, y_i is: %f", x_i, y_i);

         // send signal to watchdog every process_signal_interval
         // Send the SIGUSR1 signal to the watchdog process using the kill function. 
        if(kill(watchdog_pid, SIGUSR1) < 0)
        {
            perror("kill");
        }
        //printf("Sent signal to watchdog\n");  //add to debug
        FILE *lf_fp = fopen(logfile_name, "a");
        fprintf(lf_fp, "%f , %f , %f , %f , %f \n", F_xall, F_yall, x_i, y_i, score);
        fclose(lf_fp);
        usleep(10000);
     } 
    return 0; 
} 