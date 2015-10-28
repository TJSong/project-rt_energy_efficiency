/* NIST Secure Hash Algorithm */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sha.h"
#include "timing.h"
#include "solver.h"
#include <unistd.h>

struct slice_return sha_stream_slice(SHA_INFO *sha_info, char *file_buffer, int flen)
{
  int loop_counter[23] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int i;
  {
    sha_info->count_lo = 0L;
    return0:
    ;

  }
  int j;
  for (j = 0; j < flen; j += 8192)
  {
    loop_counter[0]++;
    i = flen - j;
    if (i > 8192)
    {
      loop_counter[1]++;
      i = 8192;
    }

    {
      int count_rename1 = i;
      if ((sha_info->count_lo + (((LONG) count_rename1) << 3)) < sha_info->count_lo)
      {
        loop_counter[2]++;
      }

      sha_info->count_lo += ((LONG) count_rename1) << 3;
      while (count_rename1 >= 64)
      {
        loop_counter[3]++;
        {
          int i_rename3;
          for (i_rename3 = 0; i_rename3 < 16; ++i_rename3)
          {
            loop_counter[4]++;
          }

          for (i_rename3 = 16; i_rename3 < 80; ++i_rename3)
          {
            loop_counter[5]++;
          }

          for (i_rename3 = 0; i_rename3 < 20; ++i_rename3)
          {
            loop_counter[6]++;
          }

          for (i_rename3 = 20; i_rename3 < 40; ++i_rename3)
          {
            loop_counter[7]++;
          }

          for (i_rename3 = 40; i_rename3 < 60; ++i_rename3)
          {
            loop_counter[8]++;
          }

          for (i_rename3 = 60; i_rename3 < 80; ++i_rename3)
          {
            loop_counter[9]++;
          }

          return3:
          ;

        }
        count_rename1 -= 64;
      }

      return1:
      ;

    }
  }

  {
    int count_rename2;
    LONG lo_bit_count_rename2;
    lo_bit_count_rename2 = sha_info->count_lo;
    count_rename2 = (int) ((lo_bit_count_rename2 >> 3) & 0x3f);
    ((BYTE *) sha_info->data)[count_rename2++] = 0x80;
    if (count_rename2 > 56)
    {
      loop_counter[10]++;
      memset(((BYTE *) (&sha_info->data)) + count_rename2, 0, 64 - count_rename2);
      {
        int i_rename4;
        for (i_rename4 = 0; i_rename4 < 16; ++i_rename4)
        {
          loop_counter[11]++;
        }

        for (i_rename4 = 16; i_rename4 < 80; ++i_rename4)
        {
          loop_counter[12]++;
        }

        for (i_rename4 = 0; i_rename4 < 20; ++i_rename4)
        {
          loop_counter[13]++;
        }

        for (i_rename4 = 20; i_rename4 < 40; ++i_rename4)
        {
          loop_counter[14]++;
        }

        for (i_rename4 = 40; i_rename4 < 60; ++i_rename4)
        {
          loop_counter[15]++;
        }

        for (i_rename4 = 60; i_rename4 < 80; ++i_rename4)
        {
          loop_counter[16]++;
        }

        return4:
        ;

      }
      memset(&sha_info->data, 0, 56);
    }
    else
    {
      memset(((BYTE *) (&sha_info->data)) + count_rename2, 0, 56 - count_rename2);
    }

    {
      int i_rename5;
      for (i_rename5 = 0; i_rename5 < 16; ++i_rename5)
      {
        loop_counter[17]++;
      }

      for (i_rename5 = 16; i_rename5 < 80; ++i_rename5)
      {
        loop_counter[18]++;
      }

      for (i_rename5 = 0; i_rename5 < 20; ++i_rename5)
      {
        loop_counter[19]++;
      }

      for (i_rename5 = 20; i_rename5 < 40; ++i_rename5)
      {
        loop_counter[20]++;
      }

      for (i_rename5 = 40; i_rename5 < 60; ++i_rename5)
      {
        loop_counter[21]++;
      }

      for (i_rename5 = 60; i_rename5 < 80; ++i_rename5)
      {
        loop_counter[22]++;
      }

      return5:
      ;

    }
    return2:
    ;

  }
  {
    print_loop_counter:
    ;
#if GET_PREDICT || DEBUG_EN
    //23
    print_array(loop_counter, sizeof(loop_counter)/sizeof(loop_counter[0]));
#endif
  }
  {
    predict_exec_time:
    ;
    struct slice_return exec_time;
    exec_time.big = exec_time.little = 0; //initialize
#if !ONLINE_EN
  #if !CVX_EN //off-line training with conservative
    exec_time.big = 0;
    exec_time.little = 80.605400*loop_counter[1] + 0.887891*loop_counter[3] + 15.467700*loop_counter[10] + 30.718300*loop_counter[17] + 0.000000;
  #else //off-line training with cvx    
    exec_time.big = 39.731974*loop_counter[0] + 39.977368*loop_counter[1] + 0.000885*loop_counter[3] + 0.000132*loop_counter[4] + 0.000036*loop_counter[5] + 0.000087*loop_counter[6] + 0.000087*loop_counter[7] + 0.000087*loop_counter[8] + 0.000087*loop_counter[9] + -2.631229*loop_counter[10] + -0.328817*loop_counter[11] + -0.116261*loop_counter[12] + -0.278069*loop_counter[13] + -0.278069*loop_counter[14] + -0.278069*loop_counter[15] + -0.278069*loop_counter[16] + 4.160141*loop_counter[17] + 1.461587*loop_counter[18] + 3.480920*loop_counter[19] + 3.480920*loop_counter[20] + 3.480920*loop_counter[21] + 3.480920*loop_counter[22] + 32.918977;
    exec_time.little = 40.301967*loop_counter[0] + 40.303426*loop_counter[1] + 0.062132*loop_counter[3] + 0.007766*loop_counter[4] + 0.002747*loop_counter[5] + 0.006571*loop_counter[6] + 0.006571*loop_counter[7] + 0.006571*loop_counter[8] + 0.006571*loop_counter[9] + 1.082202*loop_counter[10] + 0.135301*loop_counter[11] + 0.047775*loop_counter[12] + 0.114539*loop_counter[13] + 0.114539*loop_counter[14] + 0.114539*loop_counter[15] + 0.114539*loop_counter[16] + 3.947418*loop_counter[17] + 1.395629*loop_counter[18] + 3.339139*loop_counter[19] + 3.339139*loop_counter[20] + 3.339139*loop_counter[21] + 3.339139*loop_counter[22] + 31.580287;
  #endif
#elif ONLINE_EN
  #if CORE //on-line training on big core
  #else //on-line training on little core
    exec_time.little = get_predicted_time(TYPE_PREDICT, loop_counter,
        sizeof(loop_counter)/sizeof(loop_counter[0]), 0, 0);
  #endif
#endif
    return exec_time;
  }
}

int main(int argc, char **argv)
{
  FILE *fin;
  SHA_INFO sha_info;

  //---------------------modified by TJSong----------------------//
  int exec_time = 0;
  static int jump = 0;
  int pid = getpid();
  if(check_define()==ERROR_DEFINE){
      printf("%s", "DEFINE ERROR!!\n");
      return ERROR_DEFINE;
  }
#if ONLINE_EN
  init_online();
#endif
  //---------------------modified by TJSong----------------------//

  if(argc < 2){
    printf("stdin read not currently supported for slicing. Please implement.\n");
    exit(1);
    /*
       fin = stdin;
       sha_stream(&sha_info, fin);
       sha_print(&sha_info);
     */
  }else{
    while(--argc){
      fin = fopen(*(++argv), "rb");
      if(fin == NULL){
        printf("error opening %s for reading\n", *argv);
        exit(1);
      }else{
        // Get file length
        int flen;
        fseek(fin, 0, SEEK_END);
        fgetpos(fin, &flen);
        fseek(fin, 0, SEEK_SET);
        // Allocate buffer
        char *file_buffer = malloc(sizeof(char) * (flen + 1));
        // Read file into buffer
        size_t newLen = fread(file_buffer, sizeof(char), flen, fin);
        if (newLen == 0){
          printf("Error reading file\n");
          exit(1);
        }else{
          file_buffer[++newLen] = '\0';
        }
        fclose(fin);

        //---------------------modified by TJSong----------------------//
        fopen_all(); //fopen for frequnecy file
        print_deadline(DEADLINE_TIME); //print deadline 
        //---------------------modified by TJSong----------------------//

        //---------------------modified by TJSong----------------------//
        // Perform slicing and prediction
        struct slice_return predicted_exec_time;
        predicted_exec_time.big = 0;
        predicted_exec_time.little = 0;
        /*
          CASE 0 = to get prediction equation
          CASE 1 = to get execution deadline
          CASE 2 = to get overhead deadline
          CASE 3 = running on default linux governors
          CASE 4 = running on our prediction
          CASE 5 = running on oracle
          CASE 6 = running on pid
          CASE 7 = running on proactive DVFS
        */
        #if GET_PREDICT /* CASE 0 */
          predicted_exec_time = sha_stream_slice(&sha_info, file_buffer, flen);
        #elif GET_DEADLINE /* CASE 1 */
          moment_timing_print(0); //moment_start
        #elif GET_OVERHEAD /* CASE 2 */
          start_timing();
          predicted_exec_time = sha_stream_slice(&sha_info, file_buffer, flen);
          end_timing();
          slice_time = print_slice_timing();

          start_timing();
          #if CORE
            set_freq(predicted_exec_time.big, slice_time, DEADLINE_TIME, AVG_DVFS_TIME); //do dvfs
          #else
            set_freq(predicted_exec_time.little, slice_time, DEADLINE_TIME, AVG_DVFS_TIME); //do dvfs
          #endif
          end_timing();
          dvfs_time = print_dvfs_timing();
        #elif !PROACTIVE_EN && !ORACLE_EN && !PID_EN && !PREDICT_EN /* CASE 3 */
          //slice_time=0; dvfs_time=0;
          predicted_exec_time = sha_stream_slice(&sha_info, file_buffer, flen);
          moment_timing_print(0); //moment_start
        #elif !PROACTIVE_EN && !ORACLE_EN && !PID_EN && PREDICT_EN /* CASE 4 */
          moment_timing_print(0); //moment_start
          
          start_timing();
          predicted_exec_time = sha_stream_slice(&sha_info, file_buffer, flen);
          end_timing();
          slice_time = print_slice_timing();
          
          start_timing();
          #if OVERHEAD_EN //with overhead
            #if HETERO_EN
              set_freq_hetero(predicted_exec_time.big, predicted_exec_time.little, slice_time, DEADLINE_TIME, AVG_DVFS_TIME, pid); //do dvfs
            #else
              #if CORE
                set_freq(predicted_exec_time.big, slice_time, DEADLINE_TIME, AVG_DVFS_TIME); //do dvfs
              #else
                set_freq(predicted_exec_time.little, slice_time, DEADLINE_TIME, AVG_DVFS_TIME); //do dvfs
              #endif
            #endif
          #else //without overhead
            #if HETERO_EN
              set_freq_hetero(predicted_exec_time.big, predicted_exec_time.little, 0, DEADLINE_TIME, 0, pid); //do dvfs
            #else
              #if CORE
                set_freq(predicted_exec_time.big, 0, DEADLINE_TIME, 0); //do dvfs
              #else
                set_freq(predicted_exec_time.little, 0, DEADLINE_TIME, 0); //do dvfs
              #endif
            #endif
          #endif
          end_timing();
          dvfs_time = print_dvfs_timing();

          moment_timing_print(1); //moment_start
        #elif ORACLE_EN /* CASE 5 */
          //slice_time=0;
          static int job_cnt = 0; //job count
          predicted_exec_time  = exec_time_arr[job_cnt];
          moment_timing_print(0); //moment_start
          
          start_timing();
          #if CORE
            set_freq(predicted_exec_time.big, slice_time, DEADLINE_TIME, AVG_DVFS_TIME); //do dvfs
          #else
            set_freq(predicted_exec_time.little, slice_time, DEADLINE_TIME, AVG_DVFS_TIME); //do dvfs
          #endif
          end_timing();
          dvfs_time = print_dvfs_timing();
          
          moment_timing_print(1); //moment_start
          job_cnt++;
        #elif PID_EN /* CASE 6 */
          moment_timing_print(0); //moment_start
          
          start_timing();
          predicted_exec_time = pid_controller(exec_time); //pid == slice
          end_timing();
          slice_time = print_slice_timing();
          
          start_timing();
          #if CORE
            set_freq(predicted_exec_time.big, slice_time, DEADLINE_TIME, AVG_DVFS_TIME); //do dvfs
          #else
            set_freq(predicted_exec_time.little, slice_time, DEADLINE_TIME, AVG_DVFS_TIME); //do dvfs
          #endif
          end_timing();
          dvfs_time = print_dvfs_timing();
          
          moment_timing_print(1); //moment_start
        #elif PROACTIVE_EN /* CASE 7 */
          static int job_number = 0; //job count
          moment_timing_print(0); //moment_start
        
          start_timing();
          //Now, let's assume no slice time like ORACLE
          end_timing();
          slice_time = print_slice_timing();

          start_timing();
          #if HETERO_EN 
            jump = set_freq_multiple_hetero(job_number, DEADLINE_TIME, pid); //do dvfs
          #elif !HETERO_EN
            jump = set_freq_multiple(job_number, DEADLINE_TIME); //do dvfs
          #endif
          end_timing();
          dvfs_time = print_dvfs_timing();
          
          moment_timing_print(1); //moment_start
          job_number++;
        #endif

        //---------------------modified by TJSong----------------------//
        start_timing();

        //sha_stream(&sha_info, fin);
        sha_stream(&sha_info, file_buffer, flen);
        sha_print(&sha_info);

        end_timing();
        //---------------------modified by TJSong----------------------//
        exec_time = exec_timing();
        int cur_freq = print_freq(); 
        int delay_time = 0;
        int actual_delay_time = 0;
        int additional_dvfs_times = 0;
        int update_time = 0;

        #if IDLE_EN
          additional_dvfs_times =
            dvfs_table[cur_freq/100000-2][MIN_FREQ/100000-2] +
            dvfs_table[MIN_FREQ/100000-2][cur_freq/100000-2];
        #endif

        #if ONLINE_EN /* CASE 0, 2, 3 and 4 */
          #if GET_PREDICT || GET_OVERHEAD \
                || (!PROACTIVE_EN && !ORACLE_EN && !PID_EN && !PREDICT_EN) \
                || (!PROACTIVE_EN && !ORACLE_EN && !PID_EN && PREDICT_EN) 
            start_timing();
            update_time = get_predicted_time(TYPE_SOLVE, NULL, 0, exec_time,
                cur_freq);
            end_timing();
            update_time = exec_timing();
          #endif
        #endif

        #if GET_PREDICT /* CASE 0 */
          print_exec_time(exec_time);
        #elif GET_DEADLINE /* CASE 1 */
          print_exec_time(exec_time);
          moment_timing_print(2); //moment_end
        #elif GET_OVERHEAD /* CASE 2 */
          //nothing
        #else /* CASE 3, 4, 5, 6 and 7 */
          if(DELAY_EN && jump == 0 && ((delay_time = DEADLINE_TIME - exec_time 
                  - slice_time - dvfs_time - update_time 
                  - additional_dvfs_times) > 0)){
            start_timing();
            sleep_in_delay(delay_time, cur_freq);
            end_timing();
            actual_delay_time = exec_timing();
          }else
            delay_time = 0;
          moment_timing_print(2); //moment_end
          print_delay_time(delay_time, actual_delay_time);
          print_exec_time(exec_time);
          print_total_time(exec_time + slice_time + dvfs_time + actual_delay_time);
          print_update_time(update_time);
        #endif
        fclose_all();//TJSong

        // Write out predicted time & print out frequency used
        #if HETERO_EN
          print_predicted_time(predicted_exec_time.big);
          print_predicted_time(predicted_exec_time.little);
        #else
          #if CORE
            print_predicted_time(predicted_exec_time.big);
          #else
            print_predicted_time(predicted_exec_time.little);
          #endif
        #endif
        //---------------------modified by TJSong----------------------//


      }
    }
  }
  return(0);
}

