// from server of homework_5
//pipe serverのテスト　writerからの情報を受け取って描画を目指す
//todo
//watchdog実装
//
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
    double goal[5] = {1,1,1,1,1};

    //placeholders for pipes to close
    int send_pipe_writer_0_fds[2];
    int receive_pipe_writer_0_fds[2];
    int send_pipe_writer_1_fds[2];
    int receive_pipe_writer_1_fds[2];
    int send_pipe_target_0_fds[2];
    int receive_pipe_target_0_fds[2];

    // パイプの作成とエラーチェック
    if (pipe(send_pipe_writer_0_fds) == -1 || pipe(receive_pipe_writer_0_fds) == -1 ||
        pipe(send_pipe_writer_1_fds) == -1 || pipe(receive_pipe_writer_1_fds) == -1 ||
        pipe(send_pipe_target_0_fds) == -1 || pipe(receive_pipe_target_0_fds) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // ファイルディスクリプタの代入
    send_pipe_writer_0 = send_pipe_writer_0_fds[1];
    receive_pipe_writer_0 = receive_pipe_writer_0_fds[0];
    send_pipe_writer_1 = send_pipe_writer_1_fds[1];
    receive_pipe_writer_1 = receive_pipe_writer_1_fds[0];
    send_pipe_target_0 = send_pipe_target_0_fds[1];
    receive_pipe_target_0 = receive_pipe_target_0_fds[0];

    //プログラムは4つのコマンドライン引数を受け取り、各パイプのディスクリプタを設定します。
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
    //不要なパイプのクローズ
    for(int i = 0; i < 4; i++){
        close(send_pipe_read[i]);
        close(receive_pipe_write[i]);
    }

    
//受け取った情報からfdを構成
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
//最大を特定
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
    fprintf(lf_fp, "Fx, Fy, x, y \n");
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
        // 読み込み可能なデータがあるかselectで確認
        //selectは複数のファイルディスクリプタを同時に監視できる
        FD_ZERO(&reading);
        FD_SET(receive_pipe_writer_0, &reading);
        FD_SET(receive_pipe_writer_1, &reading);
        FD_SET(receive_pipe_target_0, &reading);
        int ret_val = select(max_fd + 1, &reading, NULL, NULL, NULL);

        for(int i = 0; i < max_fd + 1; i++){
            if(FD_ISSET(i, &reading)){
                //writer0からのデータを処理
                if(i == receive_pipe_writer_0)
                {
                    read(i, read_str, MSG_LEN);
                //ASK_CHAR メッセージの処理 二択あってどっちか書く
                    if (read_str[0] == ASK_CHAR){
                        // printf("writer 0 wants to write\n");
                        // printf("Writer 0 can write\n");
                        write(send_pipe_writer_0, ack_char, strlen(ack_char) + 1); 
                // ASK_CHAR以外についての処理
                    } else {
                        sscanf(read_str, "%i, %i", &tmp_cell[0], &tmp_cell[1]);
                        // printf("Writer 0 set cell 0 to %i, %i\n", tmp_cell[0],tmp_cell[1]);
                    }
                }
                else if (i == receive_pipe_writer_1)
                //writer1からのデータを処理
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
                //targetからのデータを処理
                {
                    read(i, read_str, MSG_LEN);

                    if (read_str[0] == ASK_CHAR){
                        printf("target wants to read\n");
                        //printf("Writer 1 can write\n");
                        write(send_pipe_target_0, ack_char, strlen(ack_char) + 1);
                    } else if (read_str[0] == ASKR_CHAR){
                        printf("target wants to read\n");
                        if(has_changed == 1){
                            sprintf(output_string, "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i",
                                    x_t, y_t, x_t2, y_t2, x_t3, y_t3, x_t4, y_t4, x_t5, y_t5);
                            printf("Sent %s to target \n", output_string);
                            write(send_pipe_target_0, output_string, strlen(output_string) + 1);
                            has_changed = 0;
                        }
                        else
                        {
                            printf("Value 0 has not changed\n");
                            write(send_pipe_target_0, rej_char, strlen(rej_char) + 1); 
                        }
                    } else {
                        sscanf(read_str, "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i", &tmp_cell_t[0], &tmp_cell_t[1],&tmp_cell_t[2], &tmp_cell_t[3], &tmp_cell_t[4], &tmp_cell_t[5], &tmp_cell_t[6], &tmp_cell_t[7], &tmp_cell_t[8], &tmp_cell_t[9]);
                        printf("Writer 1 set cell 1 to %i, %i, %i, %i, %i, %i, %i, %i, %i, %i \n", tmp_cell_t[0], tmp_cell_t[1], tmp_cell_t[2], tmp_cell_t[3], tmp_cell_t[4], tmp_cell_t[5], tmp_cell_t[6], tmp_cell_t[7], tmp_cell_t[8], tmp_cell_t[9]);
                    }
                }
            }
        }
        //変更されたセルの情報を更新
        for (int i = 0; i < 12; ++i){
            cell[i] = tmp_cell[i];
        }
        for (int i = 0; i < 10; ++i){
            cell_t[i] = tmp_cell_t[i];
        }

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
        x_t = cell_t[0];       
        y_t = cell_t[1];
        x_t2 = cell_t[2];
        y_t2 = cell_t[3];
        x_t3 = cell_t[4];
        y_t3 = cell_t[5];
        x_t4 = cell_t[6];
        y_t4 = cell_t[7];
        x_t5 = cell_t[8];
        y_t5 = cell_t[9];
        double F_oX = 0;
        double F_oY = 0;
        double F_tX = 0;
        double F_tY = 0;

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
        
        int lengthx = sizeof(F_ox) / sizeof(F_ox[0]);
    
        for (int i = 0; i < lengthx; i++) {
            F_oX += F_ox[i];
        }

        int lengthy = sizeof(F_oy) / sizeof(F_oy[0]);
    
        for (int i = 0; i < lengthy; i++) {
            F_oY += F_oy[i];
        }

        x_i2 = x_i1;
        x_i1 = x_i;
        x_i = ( (F_x+F_oX)*T*T +M*(2*x_i1-x_i2)+K*T*x_i1) / (M+K*T);
        //x_i = ( F_x*T*T +M*(2*x_i1-x_i2)+K*T*x_i1) / (M+K*T);
        if (x_i > MAX_NUMBER1-1){
            x_i = MAX_NUMBER1-1;
        } else if (x_i < 0){
            x_i = 0;
        }
        y_i2 = y_i1;
        y_i1 = y_i;
        y_i = ( (F_y+F_oY)*T*T +M*(2*y_i1-y_i2)+K*T*y_i1) / (M+K*T);
        //y_i = ( F_y*T*T +M*(2*y_i1-y_i2)+K*T*y_i1) / (M+K*T);
        if (y_i > MAX_NUMBER2-4){
            y_i = MAX_NUMBER2-4;
        } else if (y_i < 0){
            y_i = 0;
        }

        if (fabs(x_i - x_t) < 0.5 && fabs(y_i - y_t) < 0.5 ){
            goal[0] = 0;
            has_changed = 1;
            x_t = 500;
            y_t = 500;
        }
        if (fabs(x_i - x_t2) < 0.5 && fabs(y_i - y_t2) < 0.5 && goal[0] == 0 || goal[1] == 0){
            goal[1] = 0;
            has_changed = 1;
            x_t2 = 500;
            y_t2 = 500;
        }
        if (fabs(x_i - x_t3) < 0.5 && fabs(y_i - y_t3) < 0.5 && goal[1] == 0 || goal[2] == 0){
            goal[2] = 0;
            has_changed = 1;
            x_t3 = 500;
            y_t3 = 500;
        }
        if (fabs(x_i - x_t4) < 0.5 && fabs(y_i - y_t4) < 0.5 && goal[2] == 0 || goal[3] == 0){
            goal[3] = 0;
            has_changed = 1;
            x_t4 = 500;
            y_t4 = 500;
        }
        if (fabs(x_i - x_t5) < 0.5 && fabs(y_i - y_t5) < 0.5 && goal[3] == 0 || goal[4] == 0){
            goal[4] = 0;
            has_changed = 1;
            x_t5 = 500;
            y_t5 = 500;
        }

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
        mvprintw(MAX_NUMBER2 -2, 2, "Fx = %d , Fy = %d , x = %f , y = %f, F_oX = %f, F_oY = %f", F_x, F_y, x_i, y_i, F_oX, F_oY);
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
        fprintf(lf_fp, "%d , %d , %f , %f \n", F_x, F_y, x_i, y_i);
        fclose(lf_fp);
        usleep(10000);
     } 
    return 0; 
} 