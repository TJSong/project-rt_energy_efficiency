
/*!
 ***********************************************************************
 * \file image.c
 *
 * \brief
 *    Decode a Slice
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Inge Lille-Langoy               <inge.lille-langoy@telenor.com>
 *    - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *    - Jani Lainema                    <jani.lainema@nokia.com>
 *    - Sebastian Purreiter             <sebastian.purreiter@mch.siemens.de>
 *    - Byeong-Moon Jeon                <jeonbm@lge.com>
 *    - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 *    - Gabi Blaettermann               <blaetter@hhi.de>
 *    - Ye-Kui Wang                     <wyk@ieee.org>
 *    - Antti Hallapuro                 <antti.hallapuro@nokia.com>
 *    - Alexis Tourapis                 <alexismt@ieee.org>
 ***********************************************************************
 */

#include "contributors.h"

#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "global.h"
#include "errorconcealment.h"
#include "image.h"
#include "mbuffer.h"
#include "fmo.h"
#include "nalu.h"
#include "parsetcommon.h"
#include "parset.h"
#include "header.h"
#include "rtp.h"
#include "sei.h"
#include "output.h"
#include "biaridecod.h"
#include "mb_access.h"
#include "memalloc.h"
#include "annexb.h"

#include "context_ini.h"
#include "cabac.h"
#include "loopfilter.h"

#include "vlc.h"

#include "erc_api.h"
extern objectBuffer_t *erc_object_list;
extern ercVariables_t *erc_errorVar;
extern frame erc_recfr;
extern int erc_mvperMB;
extern struct img_par *erc_img;

//extern FILE *p_out2;

extern StorablePicture **listX[6];
extern ColocatedParams *Co_located;

StorablePicture *dec_picture;

OldSliceParams old_slice;

//---------------------modified by TJSong----------------------//
#include "timing.h"
#include "my_common.h"
//---------------------modified by TJSong----------------------//


void MbAffPostProc()
{
  imgpel temp[16][32];

  imgpel ** imgY  = dec_picture->imgY;
  imgpel ***imgUV = dec_picture->imgUV;

  int i, x, y, x0, y0, uv;
  for (i=0; i<(int)dec_picture->PicSizeInMbs; i+=2)
  {
    if (dec_picture->mb_field[i])
    {
      get_mb_pos(i, &x0, &y0);
      for (y=0; y<(2*MB_BLOCK_SIZE);y++)
        for (x=0; x<MB_BLOCK_SIZE; x++)
          temp[x][y] = imgY[y0+y][x0+x];

      for (y=0; y<MB_BLOCK_SIZE;y++)
        for (x=0; x<MB_BLOCK_SIZE; x++)
        {
          imgY[y0+(2*y)][x0+x]   = temp[x][y];
          imgY[y0+(2*y+1)][x0+x] = temp[x][y+MB_BLOCK_SIZE];
        }

      if (dec_picture->chroma_format_idc != YUV400)
      {
        x0 = x0 / (16/img->mb_cr_size_x);
        y0 = y0 / (16/img->mb_cr_size_y);

        for (uv=0; uv<2; uv++)
        {
          for (y=0; y<(2*img->mb_cr_size_y);y++)
            for (x=0; x<img->mb_cr_size_x; x++)
              temp[x][y] = imgUV[uv][y0+y][x0+x];
          
          for (y=0; y<img->mb_cr_size_y;y++)
            for (x=0; x<img->mb_cr_size_x; x++)
            {
              imgUV[uv][y0+(2*y)][x0+x]   = temp[x][y];
              imgUV[uv][y0+(2*y+1)][x0+x] = temp[x][y+img->mb_cr_size_y];
            }
        }
      }
    }
  }
}

/*!
 ***********************************************************************
 * \brief
 *    decodes one I- or P-frame
 *
 ***********************************************************************
 */

#define NOT_EOS 0
struct slice_return decode_one_frame_inner_loop_slice(struct img_par *img, struct inp_par *inp, int current_header);
int decode_one_frame(struct img_par *img,struct inp_par *inp, struct snr_par *snr, int video_index);
int decode_one_frame(struct img_par *img,struct inp_par *inp, struct snr_par *snr, int video_index)
{
  int current_header;
  Slice *currSlice = img->currentSlice;

  img->current_slice_nr = 0;
  img->current_mb_nr = -4711;     // initialized to an impossible value for debugging -- correct value is taken from slice header
  currSlice->next_header = -8888; // initialized to an impossible value for debugging -- correct value is taken from slice header
  img->num_dec_mb = 0;
  img->newframe = 1;
//---------------------modified by TJSong----------------------//
    int job_cnt = 300 * video_index; //job count
    printf("job_cnt %d\n", job_cnt); //job count
    fopen_all(); //fopen for frequnecy file
//---------------------modified by TJSong----------------------//
 
    if(check_define()==ERROR_DEFINE){
        printf("%s", "DEFINE ERROR!!\n");
        return ERROR_DEFINE;
    }

  while ((currSlice->next_header != EOS && currSlice->next_header != SOP))
  {
    int current_header;
    current_header = read_new_slice();

    if (current_header == EOS)
    {
      exit_picture();
      return EOS;
    }

    //---------------------modified by TJSong----------------------//
    print_deadline(DEADLINE_TIME); //print deadline 
    static int exec_time = 0;
    static int jump = 0;
    int pid = getpid();
    //---------------------modified by TJSong----------------------//

    // Run slicing in a forked process
    pid_t forked_pid = fork();
    if (forked_pid == 0) {
        // printf("forked %d :", job_cnt);
		// fflush(stdout);
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
            predicted_exec_time = decode_one_frame_inner_loop_slice(img, inp, current_header);
        #elif GET_DEADLINE /* CASE 1 */
            moment_timing_print(0); //moment_start
            //nothing
        #elif GET_OVERHEAD /* CASE 2 */
            start_timing();
            predicted_exec_time = decode_one_frame_inner_loop_slice(img, inp, current_header);
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
            predicted_exec_time = decode_one_frame_inner_loop_slice(img, inp, current_header);
            moment_timing_print(0); //moment_start
        #elif !PROACTIVE_EN && !ORACLE_EN && !PID_EN && PREDICT_EN /* CASE 4 */
            moment_timing_print(0); //moment_start
            
            start_timing();
            predicted_exec_time = decode_one_frame_inner_loop_slice(img, inp, current_header);
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
        #elif PROACTIVE_EN /* CASE 4 */
            moment_timing_print(0); //moment_start
          
            start_timing();
            //Now, let's assume no slice time like ORACLE
            end_timing();
            slice_time = print_slice_timing();
			 
            start_timing();
            #if HETERO_EN 
                jump = set_freq_multiple_hetero(job_cnt, DEADLINE_TIME, pid); //do dvfs
            #elif !HETERO_EN
                jump = set_freq_multiple(job_cnt, DEADLINE_TIME); //do dvfs
            #endif
            end_timing();
            dvfs_time = print_dvfs_timing();
            
            moment_timing_print(1); //moment_start
        #endif

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
      _Exit(0);
    } else {
      int status;
      waitpid(forked_pid, &status, 0);
    }
//---------------------modified by TJSong----------------------//
        job_cnt++;
//---------------------modified by TJSong----------------------//
 
    // Run job
    start_timing();
    int ret;
    ret = decode_one_frame_inner_loop(img, inp, current_header);
    if (ret == EOS) {
      return EOS;
    }
    end_timing();

    //---------------------modified by TJSong----------------------//
        exec_time = exec_timing();
        int cur_freq = print_freq(); 
        int delay_time = 0;
        int actual_delay_time = 0;
        
        #if GET_PREDICT /* CASE 0 */
            print_exec_time(exec_time);
        #elif GET_DEADLINE /* CASE 1 */
            print_exec_time(exec_time);
            moment_timing_print(2); //moment_end
        #elif GET_OVERHEAD /* CASE 2 */
            //nothing
        #else /* CASE 3,4,5 and 6 */
            if(DELAY_EN && jump == 0 && ((delay_time = DEADLINE_TIME - exec_time - slice_time - dvfs_time - dvfs_table[cur_freq/100000-2][MIN_FREQ/100000-2] - dvfs_table[MIN_FREQ/100000-2][cur_freq/100000-2]) > 0)){
                start_timing();
				sleep_in_delay(delay_time, cur_freq);
                end_timing();
                delay_time = exec_timing();
            }else
                delay_time = 0;
            moment_timing_print(2); //moment_end
            print_delay_time(delay_time, actual_delay_time);
            print_exec_time(exec_time);
            print_total_time(exec_time + slice_time + dvfs_time + actual_delay_time);
        #endif
    //---------------------modified by TJSong----------------------//

  }

//---------------------modified by TJSong----------------------//
    fclose_all();//TJSong
//---------------------modified by TJSong----------------------//
  exit_picture();

  return (SOP);
}
int decode_one_frame_inner_loop(struct img_par *img, struct inp_par *inp, int current_header) 
{
    decode_slice(img, inp, current_header);

    img->newframe = 0;
    img->current_slice_nr++;

    return NOT_EOS;
}
struct slice_return decode_one_frame_inner_loop_slice(struct img_par *img, struct inp_par *inp, int current_header)
{
  int loop_counter[42] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  Slice *currSlice = img->currentSlice;
  if (active_pps->entropy_coding_mode_flag)
  {
    loop_counter[0]++;
  }

  if (((active_pps->weighted_bipred_idc > 0) && (img->type == B_SLICE)) || (active_pps->weighted_pred_flag && (img->type != I_SLICE)))
  {
    loop_counter[1]++;
    int i_rename0;
    int j_rename0;
    int k_rename0;
    int comp_rename0;
    int td_rename0;
    int bframe_rename0 = img->type == B_SLICE;
    int max_bwd_ref_rename0;
    int max_fwd_ref_rename0;
    max_fwd_ref_rename0 = img->num_ref_idx_l0_active;
    max_bwd_ref_rename0 = img->num_ref_idx_l1_active;
    if ((active_pps->weighted_bipred_idc == 2) && bframe_rename0)
    {
      loop_counter[2]++;
      for (i_rename0 = 0; i_rename0 < MAX_REFERENCE_PICTURES; i_rename0++)
      {
        loop_counter[3]++;
        for (comp_rename0 = 0; comp_rename0 < 3; comp_rename0++)
        {
          loop_counter[4]++;
        }

      }

    }

    if (bframe_rename0)
    {
      loop_counter[5]++;
      for (i_rename0 = 0; i_rename0 < max_fwd_ref_rename0; i_rename0++)
      {
        loop_counter[6]++;
        for (j_rename0 = 0; j_rename0 < max_bwd_ref_rename0; j_rename0++)
        {
          loop_counter[7]++;
          for (comp_rename0 = 0; comp_rename0 < 3; comp_rename0++)
          {
            loop_counter[8]++;
            if (active_pps->weighted_bipred_idc == 1)
            {
              loop_counter[9]++;
            }
            else
              if (active_pps->weighted_bipred_idc == 2)
            {
              loop_counter[10]++;
              td_rename0 = Clip3(-128, 127, listX[LIST_1][j_rename0]->poc - listX[LIST_0][i_rename0]->poc);
              if (((td_rename0 == 0) || listX[LIST_1][j_rename0]->is_long_term) || listX[LIST_0][i_rename0]->is_long_term)
              {
                loop_counter[11]++;
              }
              else
              {
                if ((img->wbp_weight[1][i_rename0][j_rename0][comp_rename0] < (-64)) || (img->wbp_weight[1][i_rename0][j_rename0][comp_rename0] > 128))
                {
                  loop_counter[12]++;
                }

              }

            }


          }

        }

      }

    }

    if (bframe_rename0 && img->MbaffFrameFlag)
    {
      loop_counter[13]++;
      for (i_rename0 = 0; i_rename0 < (2 * max_fwd_ref_rename0); i_rename0++)
      {
        loop_counter[14]++;
        for (j_rename0 = 0; j_rename0 < (2 * max_bwd_ref_rename0); j_rename0++)
        {
          loop_counter[15]++;
          for (comp_rename0 = 0; comp_rename0 < 3; comp_rename0++)
          {
            loop_counter[16]++;
            for (k_rename0 = 2; k_rename0 < 6; k_rename0 += 2)
            {
              loop_counter[17]++;
              if (active_pps->weighted_bipred_idc == 1)
              {
                loop_counter[18]++;
              }
              else
                if (active_pps->weighted_bipred_idc == 2)
              {
                loop_counter[19]++;
                td_rename0 = Clip3(-128, 127, listX[k_rename0 + LIST_1][j_rename0]->poc - listX[k_rename0 + LIST_0][i_rename0]->poc);
                if (((td_rename0 == 0) || listX[k_rename0 + LIST_1][j_rename0]->is_long_term) || listX[k_rename0 + LIST_0][i_rename0]->is_long_term)
                {
                  loop_counter[20]++;
                }
                else
                {
                  if ((img->wbp_weight[k_rename0 + 1][i_rename0][j_rename0][comp_rename0] < (-64)) || (img->wbp_weight[k_rename0 + 1][i_rename0][j_rename0][comp_rename0] > 128))
                  {
                    loop_counter[21]++;
                  }

                }

              }


            }

          }

        }

      }

    }

    return0:
    ;

  }

  if (((current_header == SOP) || (current_header == SOS)) && (currSlice->ei_flag == 0))
  {
    loop_counter[22]++;
    Boolean end_of_slice_rename1 = FALSE;
    int read_flag_rename1;
    img->cod_counter = -1;
    {
      int i_rename2;
      int j_rename2;
      for (i_rename2 = 0; i_rename2 < listXsize[LIST_0]; i_rename2++)
      {
        loop_counter[23]++;
      }

      for (i_rename2 = 0; i_rename2 < listXsize[LIST_1]; i_rename2++)
      {
        loop_counter[24]++;
      }

      if (!active_sps->frame_mbs_only_flag)
      {
        loop_counter[25]++;
        if (img->structure == FRAME)
        {
          loop_counter[26]++;
          for (j_rename2 = 2; j_rename2 < 6; j_rename2++)
          {
            loop_counter[27]++;
            for (i_rename2 = 0; i_rename2 < listXsize[j_rename2]; i_rename2++)
            {
              loop_counter[28]++;
            }

          }

        }

      }

      return2:
      ;

    }
    if (img->type == B_SLICE)
    {
      loop_counter[29]++;
    }

    while (end_of_slice_rename1 == FALSE)
    {
      loop_counter[30]++;
      start_macroblock(img, inp, img->current_mb_nr);
      //read_flag_rename1 = read_one_macroblock(img, inp);
      if (img->MbaffFrameFlag && dec_picture->mb_field[img->current_mb_nr])
      {
        loop_counter[31]++;
      }

      {
        int i_rename3;
        int currMBNum_rename3 = img->current_mb_nr;
        Macroblock *currMB_rename3 = &img->mb_data[currMBNum_rename3];
        if (img->type != B_SLICE)
        {
          loop_counter[32]++;
          for (i_rename3 = 0; i_rename3 < 4; i_rename3++)
          {
            loop_counter[33]++;
            if ((currMB_rename3->b8mode[i_rename3] == 0) || (currMB_rename3->b8mode[i_rename3] == IBLOCK))
            {
              loop_counter[34]++;
            }
            else
            {
              if ((currMB_rename3->b8mode[i_rename3] >= 5) && (currMB_rename3->b8mode[i_rename3] <= 7))
              {
                loop_counter[35]++;
              }
              else
              {
              }

            }

          }

        }
        else
        {
          for (i_rename3 = 0; i_rename3 < 4; i_rename3++)
          {
            loop_counter[36]++;
            if ((currMB_rename3->mb_type == I16MB) || (currMB_rename3->b8mode[i_rename3] == IBLOCK))
            {
              loop_counter[37]++;
            }
            else
            {
            }

          }

        }

        return3:
        ;

      }
      //end_of_slice_rename1 = exit_macroblock(img, inp, (!img->MbaffFrameFlag) || (img->current_mb_nr % 2));
      img->current_mb_nr = FmoGetNextMBNr(img->current_mb_nr);
      if (img->current_mb_nr == -1) {
        break;
      }
    }

    {
      if (img->field_pic_flag)
      {
        loop_counter[38]++;
      }

      if (img->idr_flag)
      {
        loop_counter[39]++;
      }

      if (active_sps->pic_order_cnt_type == 0)
      {
        loop_counter[40]++;
      }

      if (active_sps->pic_order_cnt_type == 1)
      {
        loop_counter[41]++;
      }

      return4:
      ;

    }
    return1:
    ;

  }
  {
    print_loop_counter:
#if GET_PREDICT || DEBUG_EN
    ;

    print_array(loop_counter, 42);
#endif
    print_loop_counter_end:
    ;

  }
  {
    predict_exec_time:
    ;
    struct slice_return exec_time;
    #if !CVX_EN //conservative
        exec_time.big = 21279.800000*loop_counter[22] + -20.805600*loop_counter[34] + 7.055560*loop_counter[35] + 24952.200000*loop_counter[39] + 0.000000;
        exec_time.little = -1989.000000*loop_counter[22] + 239.500000*loop_counter[34] + -968.000000*loop_counter[39] + 0.000000;
    #else //cvx
        exec_time.big = 2473.057995*loop_counter[22] + -11446.120859*loop_counter[23] + 78.796712*loop_counter[30] + 78.796712*loop_counter[32] + 27.858832*loop_counter[33] + -25.200000*loop_counter[34] + 11.450000*loop_counter[35] + 13919.179144*loop_counter[39] + 2473.058379*loop_counter[40] + 2473.058047;
        //big dataset
		//exec_time.big = 2657.922160*loop_counter[22] + -11148.387481*loop_counter[23] + 84.694202*loop_counter[30] + 84.694202*loop_counter[32] + 29.944486*loop_counter[33] + -35.489068*loop_counter[34] + 39.036823*loop_counter[35] + 13806.416881*loop_counter[39] + 2657.939581*loop_counter[40] + 2657.923580;
        exec_time.little = -124.329431*loop_counter[22] + 8906.999995*loop_counter[23] + -9.698872*loop_counter[30] + -9.698872*loop_counter[32] + -3.429765*loop_counter[33] + 205.187500*loop_counter[34] + -69.937501*loop_counter[35] + 8776.000000*loop_counter[39] + -124.337120*loop_counter[40] + -124.329343;
        //big dataset
        //exec_time.little = 6341.306541*loop_counter[22] + -16048.028311*loop_counter[23] + 202.047523*loop_counter[30] + 202.047523*loop_counter[32] + 71.434419*loop_counter[33] + -96.615409*loop_counter[34] + -52.374843*loop_counter[35] + 22389.338986*loop_counter[39] + 6341.310303*loop_counter[40] + 6341.306435;

    #endif
/*
float exec_time;
#if CORE
    #if !CVX_EN //conservative
        exec_time = 21279.800000*loop_counter[22] + -20.805600*loop_counter[34] + 7.055560*loop_counter[35] + 24952.200000*loop_counter[39] + 0.000000;

    #else //cvx
        if(CVX_COEFF == 100)
            exec_time = 2473.057995*loop_counter[22] + -11446.120859*loop_counter[23] + 78.796712*loop_counter[30] + 78.796712*loop_counter[32] + 27.858832*loop_counter[33] + -25.200000*loop_counter[34] + 11.450000*loop_counter[35] + 13919.179144*loop_counter[39] + 2473.058379*loop_counter[40] + 2473.058047;

    #endif
#else
    #if !CVX_EN //conservative
        exec_time = -1989.000000*loop_counter[22] + 239.500000*loop_counter[34] + -968.000000*loop_counter[39] + 0.000000;

//      exec_time = -4592.000000*loop_counter[22] + 9561.000000*loop_counter[23] + 239.875000*loop_counter[34] + 1238.500000*loop_counter[35] + 4844.000000*loop_counter[39] + 0.000000;
    #else //cvx
        if(CVX_COEFF == 1)
            exec_time = 966.509713*loop_counter[22] + 7271.973596*loop_counter[23] + 75.420060*loop_counter[30] + 75.420060*loop_counter[32] + 26.668309*loop_counter[33] + 68.434206*loop_counter[34] + 49.815775*loop_counter[35] + 14909.508053*loop_counter[39] + 966.503992*loop_counter[40] + 966.507252;
        else if(CVX_COEFF == 2)
            exec_time = 1648.196134*loop_counter[22] + 5912.276550*loop_counter[23] + 128.597565*loop_counter[30] + 128.597565*loop_counter[32] + 45.469064*loop_counter[33] + 47.750005*loop_counter[34] + -50.222189*loop_counter[35] + 12550.276586*loop_counter[39] + 1648.191065*loop_counter[40] + 1648.194021;
        else if(CVX_COEFF == 5)
            exec_time = 584.636639*loop_counter[22] + 7040.999993*loop_counter[23] + 45.621318*loop_counter[30] + 45.621318*loop_counter[32] + 16.131796*loop_counter[33] + 157.749998*loop_counter[34] + 277.899993*loop_counter[35] + 11278.999999*loop_counter[39] + 584.633765*loop_counter[40] + 584.635190;
        else if(CVX_COEFF == 10)
            exec_time = 588.489841*loop_counter[22] + 6880.000011*loop_counter[23] + 45.919743*loop_counter[30] + 45.919743*loop_counter[32] + 16.236245*loop_counter[33] + 169.150000*loop_counter[34] + 539.436366*loop_counter[35] + 12436.999996*loop_counter[39] + 588.487489*loop_counter[40] + 588.488648;
        else if(CVX_COEFF == 100)
            exec_time = 54.829064*loop_counter[22] + 404.413895*loop_counter[23] + 4.277229*loop_counter[30] + 4.277229*loop_counter[32] + 1.512213*loop_counter[33] + 212.750000*loop_counter[34] + -349.586093*loop_counter[39] + 54.828945*loop_counter[40] + 54.828770;
        
//    exec_time = 1381.236055*loop_counter[22] + 10991.000000*loop_counter[23] + 107.751799*loop_counter[30] + 107.751799*loop_counter[32] + 38.097948*loop_counter[33] + 96.553797*loop_counter[34] + 275.721519*loop_counter[35] + 12437.000000*loop_counter[39] + 1381.210955*loop_counter[40] + 1381.235647;
        else if(CVX_COEFF == 1000)
            exec_time = 215.060560*loop_counter[22] + 14365.999999*loop_counter[23] + 16.780752*loop_counter[30] + 16.780752*loop_counter[32] + 5.933844*loop_counter[33] + 203.357595*loop_counter[34] + 318.443038*loop_counter[35] + 12437.000000*loop_counter[39] + 215.061103*loop_counter[40] + 215.060472;

    #endif
#endif
*/
    return exec_time;
  }
}

/*!
 ************************************************************************
 * \brief
 *    Convert file read buffer to source picture structure
 * \param imgX
 *    Pointer to image plane
 * \param buf
 *    Buffer for file output
 * \param size
 *    image size in pixel
 ************************************************************************
 */
void buf2img (imgpel** imgX, unsigned char* buf, int size_x, int size_y, int symbol_size_in_bytes)
{
  int i,j;

  unsigned short tmp16, ui16;
  unsigned long  tmp32, ui32;

  if (symbol_size_in_bytes> sizeof(imgpel))
  {
    error ("Source picture has higher bit depth than imgpel data type. Please recompile with larger data type for imgpel.", 500);
  }

  if (( sizeof(char) == sizeof (imgpel)) && ( sizeof(char) == symbol_size_in_bytes))
  {
    // imgpel == pixel_in_file == 1 byte -> simple copy
    for(j=0;j<size_y;j++)
      memcpy(imgX[j], buf+j*size_x, size_x);
  }
  else
  {
    // sizeof (imgpel) > sizeof(char)
    if (testEndian())
    {
      // big endian
      switch (symbol_size_in_bytes)
      {
      case 1:
        {
          for(j=0;j<size_y;j++)
            for(i=0;i<size_x;i++)
            {
              imgX[j][i]= buf[i+j*size_x];
            }
          break;
        }
      case 2:
        {
          for(j=0;j<size_y;j++)
            for(i=0;i<size_x;i++)
            {
              memcpy(&tmp16, buf+((i+j*size_x)*2), 2);
              ui16  = (tmp16 >> 8) | ((tmp16&0xFF)<<8);
              imgX[j][i] = (imgpel) ui16;
            }
          break;
        }
      case 4:
        {
          for(j=0;j<size_y;j++)
            for(i=0;i<size_x;i++)
            {
              memcpy(&tmp32, buf+((i+j*size_x)*4), 4);
              ui32  = ((tmp32&0xFF00)<<8) | ((tmp32&0xFF)<<24) | ((tmp32&0xFF0000)>>8) | ((tmp32&0xFF000000)>>24);
              imgX[j][i] = (imgpel) ui32;
            }
        }
      default:
        {
           error ("reading only from formats of 8, 16 or 32 bit allowed on big endian architecture", 500);
           break;
        }
      }

    }
    else
    {
      // little endian
      for (j=0; j < size_y; j++)
        for (i=0; i < size_x; i++)
        {
          imgX[j][i]=0;
          memcpy(&(imgX[j][i]), buf +((i+j*size_x)*symbol_size_in_bytes), symbol_size_in_bytes);
        }

    }
  }
}


/*!
************************************************************************
* \brief
*    Find PSNR for all three components.Compare decoded frame with
*    the original sequence. Read inp->jumpd frames to reflect frame skipping.
************************************************************************
*/
void find_snr(
              struct snr_par  *snr,   //!< pointer to snr parameters
              StorablePicture *p,     //!< picture to be compared
              int p_ref)              //!< open reference YUV file
{
  int SubWidthC  [4]= { 1, 2, 2, 1};
  int SubHeightC [4]= { 1, 2, 1, 1};
  int crop_left, crop_right, crop_top, crop_bottom;

  int i,j;
  int64 diff_y,diff_u,diff_v;
  int uv;
  int64  status;
  int symbol_size_in_bytes = img->pic_unit_bitsize_on_disk/8;
  int size_x, size_y;
  int size_x_cr, size_y_cr;
  int64 framesize_in_bytes;
  unsigned int max_pix_value_sqd = img->max_imgpel_value * img->max_imgpel_value;
  unsigned int max_pix_value_sqd_uv = img->max_imgpel_value_uv * img->max_imgpel_value_uv;
  Boolean rgb_output = (active_sps->vui_seq_parameters.matrix_coefficients==0);
  unsigned char *buf;

  // calculate frame number
  int  psnrPOC = active_sps->mb_adaptive_frame_field_flag ? p->poc /(input->poc_scale) : p->poc/(3-input->poc_scale);

  // cropping for luma
  if (p->frame_cropping_flag)
  {
    crop_left   = SubWidthC[p->chroma_format_idc] * p->frame_cropping_rect_left_offset;
    crop_right  = SubWidthC[p->chroma_format_idc] * p->frame_cropping_rect_right_offset;
    crop_top    = SubHeightC[p->chroma_format_idc]*( 2 - p->frame_mbs_only_flag ) *  p->frame_cropping_rect_top_offset;
    crop_bottom = SubHeightC[p->chroma_format_idc]*( 2 - p->frame_mbs_only_flag ) *   p->frame_cropping_rect_bottom_offset;
  }
  else
  {
    crop_left = crop_right = crop_top = crop_bottom = 0;
  }

  size_x = p->size_x - crop_left - crop_right;
  size_y = p->size_y - crop_top - crop_bottom;

  // cropping for chroma
  if (p->frame_cropping_flag)
  {
    crop_left   = p->frame_cropping_rect_left_offset;
    crop_right  = p->frame_cropping_rect_right_offset;
    crop_top    = ( 2 - p->frame_mbs_only_flag ) *  p->frame_cropping_rect_top_offset;
    crop_bottom = ( 2 - p->frame_mbs_only_flag ) *   p->frame_cropping_rect_bottom_offset;
  }
  else
  {
    crop_left = crop_right = crop_top = crop_bottom = 0;
  }

  if ((p->chroma_format_idc==YUV400) && input->write_uv)
  {
    size_x_cr = p->size_x/2;
    size_y_cr = p->size_y/2;
  }
  else
  {
    size_x_cr = p->size_x_cr - crop_left - crop_right;
    size_y_cr = p->size_y_cr - crop_top  - crop_bottom;
  }

  framesize_in_bytes = (((int64)size_y*size_x) + ((int64)size_y_cr*size_x_cr)*2) * symbol_size_in_bytes;

  if (psnrPOC==0 && img->psnr_number)
    img->idr_psnr_number=img->psnr_number + 1;

  img->psnr_number=max(img->psnr_number,img->idr_psnr_number+psnrPOC);

  frame_no = img->idr_psnr_number+psnrPOC;

  // KS: this buffer should actually be allocated only once, but this is still much faster than the previous version
  buf = malloc ( size_y * size_x * symbol_size_in_bytes );

  if (NULL == buf)
  {
    no_mem_exit("find_snr: buf");
  }

  status = lseek (p_ref, framesize_in_bytes * frame_no, SEEK_SET);
  if (status == -1)
  {
    fprintf(stderr, "Error in seeking frame number: %d\n", frame_no);
    free (buf);
    return;
  }

  if(rgb_output)
    lseek (p_ref, framesize_in_bytes/3, SEEK_CUR);

  read(p_ref, buf, size_y * size_x * symbol_size_in_bytes);
  buf2img(imgY_ref, buf, size_x, size_y, symbol_size_in_bytes);

  if (p->chroma_format_idc != YUV400)
  {
    for (uv=0; uv < 2; uv++)
    {
      if(rgb_output && uv==1)
        lseek (p_ref, -framesize_in_bytes, SEEK_CUR);
    
      read(p_ref, buf, size_y_cr * size_x_cr*symbol_size_in_bytes);
      buf2img(imgUV_ref[uv], buf, size_x_cr, size_y_cr, symbol_size_in_bytes);
    }
  }

   if(rgb_output) 
     lseek (p_ref, framesize_in_bytes*2/3, SEEK_CUR);
  
  free (buf);

  img->quad[0]=0;
  diff_y=0;
  for (j=0; j < size_y; ++j)
  {
    for (i=0; i < size_x; ++i)
    {
      diff_y += img->quad[abs(p->imgY[j][i]-imgY_ref[j][i])];
    }
  }

  // Chroma
  diff_u=0;
  diff_v=0;
  
  if (p->chroma_format_idc != YUV400)
  {
    for (j=0; j < size_y_cr; ++j)
    {
      for (i=0; i < size_x_cr; ++i)
      {
        diff_u += img->quad[abs(imgUV_ref[0][j][i]-p->imgUV[0][j][i])];
        diff_v += img->quad[abs(imgUV_ref[1][j][i]-p->imgUV[1][j][i])];
      }
    }
  }

#if ZEROSNR
  if (diff_y == 0)
    diff_y = 1;
  if (diff_u == 0)
    diff_u = 1;
  if (diff_v == 0)
    diff_v = 1; 
#endif

  // Collecting SNR statistics
  if (diff_y != 0)
    snr->snr_y=(float)(10*log10(max_pix_value_sqd*(double)((double)(size_x)*(size_y) / diff_y)));        // luma snr for current frame
  else
    snr->snr_y=0;
  if (diff_u != 0)
    snr->snr_u=(float)(10*log10(max_pix_value_sqd_uv*(double)((double)(size_x_cr)*(size_y_cr) / (diff_u))));    //  chroma snr for current frame
  else
    snr->snr_u=0;
  if (diff_v != 0)
    snr->snr_v=(float)(10*log10(max_pix_value_sqd_uv*(double)((double)(size_x_cr)*(size_y_cr) / (diff_v))));    //  chroma snr for current frame
  else
    snr->snr_v=0;

  if (img->number == 0) // first
  {
    snr->snr_ya=snr->snr_y1=snr->snr_y;                                                        // keep luma snr for first frame
    snr->snr_ua=snr->snr_u1=snr->snr_u;                                                        // keep chroma snr for first frame
    snr->snr_va=snr->snr_v1=snr->snr_v;                                                        // keep chroma snr for first frame
  
  }
  else
  {
    snr->snr_ya=(float)(snr->snr_ya*(snr->frame_ctr)+snr->snr_y)/(snr->frame_ctr+1); // average snr chroma for all frames
    snr->snr_ua=(float)(snr->snr_ua*(snr->frame_ctr)+snr->snr_u)/(snr->frame_ctr+1); // average snr luma for all frames
    snr->snr_va=(float)(snr->snr_va*(snr->frame_ctr)+snr->snr_v)/(snr->frame_ctr+1); // average snr luma for all frames
  } 
}


/*!
 ************************************************************************
 * \brief
 *    Interpolation of 1/4 subpixel
 ************************************************************************
 */
void get_block(int ref_frame, StorablePicture **list, int x_pos, int y_pos, struct img_par *img, int block[BLOCK_SIZE][BLOCK_SIZE])
{

  int dx, dy;
  int x, y;
  int i, j;
  int maxold_x,maxold_y;
  int result;
  int pres_x;
  int pres_y; 
  int tmp_res[4][9];
  static const int COEF[6] = {    1, -5, 20, 20, -5, 1  };

  dx = x_pos&3;
  dy = y_pos&3;
  x_pos = (x_pos-dx)/4;
  y_pos = (y_pos-dy)/4;

  maxold_x = dec_picture->size_x-1;
  maxold_y = dec_picture->size_y-1;

  if (dec_picture->mb_field[img->current_mb_nr])
    maxold_y = dec_picture->size_y/2 - 1;

  if (dx == 0 && dy == 0) 
  {  /* fullpel position */
    for (j = 0; j < BLOCK_SIZE; j++)
      for (i = 0; i < BLOCK_SIZE; i++)
        block[i][j] = list[ref_frame]->imgY[max(0,min(maxold_y,y_pos+j))][max(0,min(maxold_x,x_pos+i))];
  }
  else 
  { /* other positions */

    if (dy == 0) 
    { /* No vertical interpolation */

      for (j = 0; j < BLOCK_SIZE; j++) 
      {
        for (i = 0; i < BLOCK_SIZE; i++) 
        {
          for (result = 0, x = -2; x < 4; x++)
            result += list[ref_frame]->imgY[max(0,min(maxold_y,y_pos+j))][max(0,min(maxold_x,x_pos+i+x))]*COEF[x+2];
          block[i][j] = max(0, min(img->max_imgpel_value, (result+16)/32));
        }
      }

      if ((dx&1) == 1) 
      {
        for (j = 0; j < BLOCK_SIZE; j++)
          for (i = 0; i < BLOCK_SIZE; i++)
            block[i][j] = (block[i][j] + list[ref_frame]->imgY[max(0,min(maxold_y,y_pos+j))][max(0,min(maxold_x,x_pos+i+dx/2))] +1 )/2;
      }
    }
    else if (dx == 0) 
    {  /* No horizontal interpolation */

      for (j = 0; j < BLOCK_SIZE; j++) 
      {
        for (i = 0; i < BLOCK_SIZE; i++) 
        {
          for (result = 0, y = -2; y < 4; y++)
            result += list[ref_frame]->imgY[max(0,min(maxold_y,y_pos+j+y))][max(0,min(maxold_x,x_pos+i))]*COEF[y+2];
          block[i][j] = max(0, min(img->max_imgpel_value, (result+16)/32));
        }
      }

      if ((dy&1) == 1) 
      {
        for (j = 0; j < BLOCK_SIZE; j++)
          for (i = 0; i < BLOCK_SIZE; i++)
           block[i][j] = (block[i][j] + list[ref_frame]->imgY[max(0,min(maxold_y,y_pos+j+dy/2))][max(0,min(maxold_x,x_pos+i))] +1 )/2;
      }
    }
    else if (dx == 2) 
    {  /* Vertical & horizontal interpolation */

      for (j = -2; j < BLOCK_SIZE+3; j++) 
      {
        for (i = 0; i < BLOCK_SIZE; i++)
          for (tmp_res[i][j+2] = 0, x = -2; x < 4; x++)
            tmp_res[i][j+2] += list[ref_frame]->imgY[max(0,min(maxold_y,y_pos+j))][max(0,min(maxold_x,x_pos+i+x))]*COEF[x+2];
      }

      for (j = 0; j < BLOCK_SIZE; j++) 
      {
        for (i = 0; i < BLOCK_SIZE; i++) 
        {
          for (result = 0, y = -2; y < 4; y++)
            result += tmp_res[i][j+y+2]*COEF[y+2];
          block[i][j] = max(0, min(img->max_imgpel_value, (result+512)/1024));
        } 
      }

      if ((dy&1) == 1)
      {
        for (j = 0; j < BLOCK_SIZE; j++)
          for (i = 0; i < BLOCK_SIZE; i++)
            block[i][j] = (block[i][j] + max(0, min(img->max_imgpel_value, (tmp_res[i][j+2+dy/2]+16)/32)) +1 )/2;
      }
    }
    else if (dy == 2)
    {  /* Horizontal & vertical interpolation */

      for (j = 0; j < BLOCK_SIZE; j++)
      {
        for (i = -2; i < BLOCK_SIZE+3; i++)
          for (tmp_res[j][i+2] = 0, y = -2; y < 4; y++)
            tmp_res[j][i+2] += list[ref_frame]->imgY[max(0,min(maxold_y,y_pos+j+y))][max(0,min(maxold_x,x_pos+i))]*COEF[y+2];
      }

      for (j = 0; j < BLOCK_SIZE; j++)
      {
        for (i = 0; i < BLOCK_SIZE; i++)
        {
          for (result = 0, x = -2; x < 4; x++)
            result += tmp_res[j][i+x+2]*COEF[x+2];
          block[i][j] = max(0, min(img->max_imgpel_value, (result+512)/1024));
        }
      }

      if ((dx&1) == 1)
      {
        for (j = 0; j < BLOCK_SIZE; j++)
          for (i = 0; i < BLOCK_SIZE; i++)
            block[i][j] = (block[i][j] + max(0, min(img->max_imgpel_value, (tmp_res[j][i+2+dx/2]+16)/32))+1)/2;
      }
    }
    else
    {  /* Diagonal interpolation */

      for (j = 0; j < BLOCK_SIZE; j++)
      {
        for (i = 0; i < BLOCK_SIZE; i++)
        {
          pres_y = dy == 1 ? y_pos+j : y_pos+j+1;
          pres_y = max(0,min(maxold_y,pres_y));
          for (result = 0, x = -2; x < 4; x++)
            result += list[ref_frame]->imgY[pres_y][max(0,min(maxold_x,x_pos+i+x))]*COEF[x+2];
          block[i][j] = max(0, min(img->max_imgpel_value, (result+16)/32));
        }
      }

      for (j = 0; j < BLOCK_SIZE; j++)
      {
        for (i = 0; i < BLOCK_SIZE; i++)
        {
          pres_x = dx == 1 ? x_pos+i : x_pos+i+1;
          pres_x = max(0,min(maxold_x,pres_x));
          for (result = 0, y = -2; y < 4; y++)
            result += list[ref_frame]->imgY[max(0,min(maxold_y,y_pos+j+y))][pres_x]*COEF[y+2];
          block[i][j] = (block[i][j] + max(0, min(img->max_imgpel_value, (result+16)/32)) +1 ) / 2;
        }
      }

    }
  }
}


void reorder_lists(int currSliceType, Slice * currSlice)
{

  if ((currSliceType != I_SLICE)&&(currSliceType != SI_SLICE))
  {
    if (currSlice->ref_pic_list_reordering_flag_l0)
    {
      reorder_ref_pic_list(listX[0], &listXsize[0], 
                           img->num_ref_idx_l0_active - 1, 
                           currSlice->remapping_of_pic_nums_idc_l0, 
                           currSlice->abs_diff_pic_num_minus1_l0, 
                           currSlice->long_term_pic_idx_l0);
    }
    if (NULL == listX[0][img->num_ref_idx_l0_active-1])
    {
      error("RefPicList0[ num_ref_idx_l0_active_minus1 ] is equal to 'no reference picture', invalid bitstream",500);
    }
    // that's a definition
    listXsize[0] = img->num_ref_idx_l0_active;
  }
  if (currSliceType == B_SLICE)
  {
    if (currSlice->ref_pic_list_reordering_flag_l1)
    {
      reorder_ref_pic_list(listX[1], &listXsize[1], 
                           img->num_ref_idx_l1_active - 1, 
                           currSlice->remapping_of_pic_nums_idc_l1, 
                           currSlice->abs_diff_pic_num_minus1_l1, 
                           currSlice->long_term_pic_idx_l1);
    }
    if (NULL == listX[1][img->num_ref_idx_l1_active-1])
    {
      error("RefPicList1[ num_ref_idx_l1_active_minus1 ] is equal to 'no reference picture', invalid bitstream",500);
    }
    // that's a definition
    listXsize[1] = img->num_ref_idx_l1_active;
  }

  free_ref_pic_list_reordering_buffer(currSlice);
}


/*!
 ************************************************************************
 * \brief
 *    initialize ref_pic_num array
 ************************************************************************
 */
void set_ref_pic_num()
{
  int i,j;

  int slice_id=img->current_slice_nr;

  for (i=0;i<listXsize[LIST_0];i++)
  {
    dec_picture->ref_pic_num        [slice_id][LIST_0][i]=listX[LIST_0][i]->poc * 2 + ((listX[LIST_0][i]->structure==BOTTOM_FIELD)?1:0) ; 
    dec_picture->frm_ref_pic_num    [slice_id][LIST_0][i]=listX[LIST_0][i]->frame_poc * 2; 
    dec_picture->top_ref_pic_num    [slice_id][LIST_0][i]=listX[LIST_0][i]->top_poc * 2; 
    dec_picture->bottom_ref_pic_num [slice_id][LIST_0][i]=listX[LIST_0][i]->bottom_poc * 2 + 1; 
    //printf("POCS %d %d %d %d ",listX[LIST_0][i]->frame_poc,listX[LIST_0][i]->bottom_poc,listX[LIST_0][i]->top_poc,listX[LIST_0][i]->poc);
    //printf("refid %d %d %d %d\n",(int) dec_picture->frm_ref_pic_num[LIST_0][i],(int) dec_picture->top_ref_pic_num[LIST_0][i],(int) dec_picture->bottom_ref_pic_num[LIST_0][i],(int) dec_picture->ref_pic_num[LIST_0][i]);
  }

  for (i=0;i<listXsize[LIST_1];i++)
  {
    dec_picture->ref_pic_num        [slice_id][LIST_1][i]=listX[LIST_1][i]->poc  *2 + ((listX[LIST_1][i]->structure==BOTTOM_FIELD)?1:0);
    dec_picture->frm_ref_pic_num    [slice_id][LIST_1][i]=listX[LIST_1][i]->frame_poc * 2; 
    dec_picture->top_ref_pic_num    [slice_id][LIST_1][i]=listX[LIST_1][i]->top_poc * 2; 
    dec_picture->bottom_ref_pic_num [slice_id][LIST_1][i]=listX[LIST_1][i]->bottom_poc * 2 + 1; 
  }

  if (!active_sps->frame_mbs_only_flag)
  {
    if (img->structure==FRAME)
      for (j=2;j<6;j++)
        for (i=0;i<listXsize[j];i++)
        {
          dec_picture->ref_pic_num        [slice_id][j][i] = listX[j][i]->poc * 2 + ((listX[j][i]->structure==BOTTOM_FIELD)?1:0);
          dec_picture->frm_ref_pic_num    [slice_id][j][i] = listX[j][i]->frame_poc * 2 ;
          dec_picture->top_ref_pic_num    [slice_id][j][i] = listX[j][i]->top_poc * 2 ;
          dec_picture->bottom_ref_pic_num [slice_id][j][i] = listX[j][i]->bottom_poc * 2 + 1;
        }
  }

}


/*!
 ************************************************************************
 * \brief
 *    Reads new slice from bit_stream
 ************************************************************************
 */
int read_new_slice()
{
  NALU_t *nalu = AllocNALU(MAX_CODED_FRAME_SIZE);
  int current_header;
  int ret;
  int BitsUsedByHeader;
  Slice *currSlice = img->currentSlice;
  Bitstream *currStream;

  int slice_id_a, slice_id_b, slice_id_c;
  int redundant_pic_cnt_b, redundant_pic_cnt_c;
  long ftell_position, expected_slice_type;
  
//  int i;
  expected_slice_type = NALU_TYPE_DPA;

  while (1)
  {
    ftell_position = ftell(bits);

    if (input->FileFormat == PAR_OF_ANNEXB)
      ret=GetAnnexbNALU (nalu);
    else
      ret=GetRTPNALU (nalu);

    //In some cases, zero_byte shall be present. If current NALU is a VCL NALU, we can't tell
    //whether it is the first VCL NALU at this point, so only non-VCL NAL unit is checked here.
    CheckZeroByteNonVCL(nalu, &ret);

    NALUtoRBSP(nalu);
//    printf ("nalu->len %d\n", nalu->len);
    
    if (ret < 0)
      printf ("Error while getting the NALU in file format %s, exit\n", input->FileFormat==PAR_OF_ANNEXB?"Annex B":"RTP");
    if (ret == 0)
    {
//      printf ("read_new_slice: returning %s\n", "EOS");
      if(expected_slice_type != NALU_TYPE_DPA)
      {
        /* oops... we found the next slice, go back! */
        fseek(bits, ftell_position, SEEK_SET);
        FreeNALU(nalu);
        return current_header;
      }
      else
        return EOS;
    }

    // Got a NALU
    if (nalu->forbidden_bit)
    {
      printf ("Found NALU w/ forbidden_bit set, bit error?  Let's try...\n");
    }

    switch (nalu->nal_unit_type)
    {
      case NALU_TYPE_SLICE:
      case NALU_TYPE_IDR:
        img->idr_flag = (nalu->nal_unit_type == NALU_TYPE_IDR);
        img->nal_reference_idc = nalu->nal_reference_idc;
        img->disposable_flag = (nalu->nal_reference_idc == NALU_PRIORITY_DISPOSABLE);
        currSlice->dp_mode = PAR_DP_1;
        currSlice->max_part_nr = 1;
        currSlice->ei_flag = 0;
        currStream = currSlice->partArr[0].bitstream;
        currStream->ei_flag = 0;
        currStream->frame_bitoffset = currStream->read_len = 0;
        memcpy (currStream->streamBuffer, &nalu->buf[1], nalu->len-1);
        currStream->code_len = currStream->bitstream_length = RBSPtoSODB(currStream->streamBuffer, nalu->len-1);

        // Some syntax of the Slice Header depends on the parameter set, which depends on
        // the parameter set ID of the SLice header.  Hence, read the pic_parameter_set_id
        // of the slice header first, then setup the active parameter sets, and then read
        // the rest of the slice header
        BitsUsedByHeader = FirstPartOfSliceHeader();
        UseParameterSet (currSlice->pic_parameter_set_id);
        BitsUsedByHeader+= RestOfSliceHeader ();

        FmoInit (active_pps, active_sps);

        AssignQuantParam (active_pps, active_sps);

        if(is_new_picture())
        {
          init_picture(img, input);
          
          current_header = SOP;
          //check zero_byte if it is also the first NAL unit in the access unit
          CheckZeroByteVCL(nalu, &ret);
        }
        else
          current_header = SOS;
  
        init_lists(img->type, img->currentSlice->structure);
        reorder_lists (img->type, img->currentSlice);

        if (img->structure==FRAME)
        {
          init_mbaff_lists();
        }

/*        if (img->frame_num==1) // write a reference list
        {
          count ++;
          if (count==1)
            for (i=0; i<listXsize[0]; i++)
              write_picture(listX[0][i], p_out2);
        }
*/

        // From here on, active_sps, active_pps and the slice header are valid
        if (img->MbaffFrameFlag)
          img->current_mb_nr = currSlice->start_mb_nr << 1;
        else
          img->current_mb_nr = currSlice->start_mb_nr;

        if (active_pps->entropy_coding_mode_flag)
        {
          int ByteStartPosition = currStream->frame_bitoffset/8;
          if (currStream->frame_bitoffset%8 != 0) 
          {
            ByteStartPosition++;
          }
          arideco_start_decoding (&currSlice->partArr[0].de_cabac, currStream->streamBuffer, ByteStartPosition, &currStream->read_len, img->type);
        }
// printf ("read_new_slice: returning %s\n", current_header == SOP?"SOP":"SOS");
        FreeNALU(nalu);
        return current_header;
        break;
      case NALU_TYPE_DPA:
        //! The state machine here should follow the same ideas as the old readSliceRTP()
        //! basically:
        //! work on DPA (as above)
        //! read and process all following SEI/SPS/PPS/PD/Filler NALUs
        //! if next video NALU is dpB, 
        //!   then read and check whether it belongs to DPA, if yes, use it
        //! else
        //!   ;   // nothing
        //! read and process all following SEI/SPS/PPS/PD/Filler NALUs
        //! if next video NALU is dpC
        //!   then read and check whether it belongs to DPA (and DPB, if present), if yes, use it, done
        //! else
        //!   use the DPA (and the DPB if present)

        /* 
            LC: inserting the code related to DP processing, mainly copying some of the parts
            related to NALU_TYPE_SLICE, NALU_TYPE_IDR.
        */

        if(expected_slice_type != NALU_TYPE_DPA)
        {
          /* oops... we found the next slice, go back! */
          fseek(bits, ftell_position, SEEK_SET);
          FreeNALU(nalu);
          return current_header;
        }

        img->idr_flag          = (nalu->nal_unit_type == NALU_TYPE_IDR);
        img->nal_reference_idc = nalu->nal_reference_idc;
        img->disposable_flag   = (nalu->nal_reference_idc == NALU_PRIORITY_DISPOSABLE);
        currSlice->dp_mode     = PAR_DP_3;
        currSlice->max_part_nr = 3;
        currSlice->ei_flag     = 0;
        currStream             = currSlice->partArr[0].bitstream;
        currStream->ei_flag    = 0;
        currStream->frame_bitoffset = currStream->read_len = 0;
        memcpy (currStream->streamBuffer, &nalu->buf[1], nalu->len-1);
        currStream->code_len = currStream->bitstream_length = RBSPtoSODB(currStream->streamBuffer, nalu->len-1);
        
        BitsUsedByHeader     = FirstPartOfSliceHeader();
        UseParameterSet (currSlice->pic_parameter_set_id);
        BitsUsedByHeader    += RestOfSliceHeader ();
        
        FmoInit (active_pps, active_sps);
        
        if(is_new_picture())
        {
          init_picture(img, input);
          current_header = SOP;
          CheckZeroByteVCL(nalu, &ret);
        }
        else
          current_header = SOS;

        
        init_lists(img->type, img->currentSlice->structure);
        reorder_lists (img->type, img->currentSlice);
        
        if (img->structure==FRAME)
        {
          init_mbaff_lists();
        }

        // From here on, active_sps, active_pps and the slice header are valid
        if (img->MbaffFrameFlag)
          img->current_mb_nr = currSlice->start_mb_nr << 1;
        else
          img->current_mb_nr = currSlice->start_mb_nr;


        /* 
           LC:
              Now I need to read the slice ID, which depends on the value of 
              redundant_pic_cnt_present_flag (pag.49). 
        */
        
        slice_id_a  = ue_v("NALU:SLICE_A slice_idr", currStream);
        if (active_pps->entropy_coding_mode_flag)
        {
          int ByteStartPosition = currStream->frame_bitoffset/8;
          if (currStream->frame_bitoffset%8 != 0) 
          {
            ByteStartPosition++;
          }
          arideco_start_decoding (&currSlice->partArr[0].de_cabac, currStream->streamBuffer, ByteStartPosition, &currStream->read_len, img->type);
        }
// printf ("read_new_slice: returning %s\n", current_header == SOP?"SOP":"SOS");
        break;
      case NALU_TYPE_DPB:
        /* LC: inserting the code related to DP processing */

        currStream             = currSlice->partArr[1].bitstream;
        currStream->ei_flag    = 0;
        currStream->frame_bitoffset = currStream->read_len = 0;
        memcpy (currStream->streamBuffer, &nalu->buf[1], nalu->len-1);
        currStream->code_len = currStream->bitstream_length = RBSPtoSODB(currStream->streamBuffer, nalu->len-1);

        slice_id_b  = ue_v("NALU:SLICE_B slice_idr", currStream);
        if (active_pps->redundant_pic_cnt_present_flag)
          redundant_pic_cnt_b = ue_v("NALU:SLICE_B redudand_pic_cnt", currStream);
        else
          redundant_pic_cnt_b = 0;
        
        /*  LC: Initializing CABAC for the current data stream. */

        if (active_pps->entropy_coding_mode_flag)
        {
          int ByteStartPosition = currStream->frame_bitoffset/8;
          if (currStream->frame_bitoffset % 8 != 0) 
            ByteStartPosition++;
          
          arideco_start_decoding (&currSlice->partArr[1].de_cabac, currStream->streamBuffer, 
            ByteStartPosition, &currStream->read_len, img->type);
          
        }

        /* LC: resilience code to be inserted */
        /*         FreeNALU(nalu); */
        /*         return current_header; */

        break;
      case NALU_TYPE_DPC:
        /* LC: inserting the code related to DP processing */
        currStream             = currSlice->partArr[2].bitstream;
        currStream->ei_flag    = 0;
        currStream->frame_bitoffset = currStream->read_len = 0;
        memcpy (currStream->streamBuffer, &nalu->buf[1], nalu->len-1);
        currStream->code_len = currStream->bitstream_length = RBSPtoSODB(currStream->streamBuffer, nalu->len-1);
        
        slice_id_c  = ue_v("NALU:SLICE_C slice_idr", currStream);
        if (active_pps->redundant_pic_cnt_present_flag)
          redundant_pic_cnt_c = ue_v("NALU:SLICE_C redudand_pic_cnt", currStream);
        else
          redundant_pic_cnt_c = 0;
        
        /* LC: Initializing CABAC for the current data stream. */

        if (active_pps->entropy_coding_mode_flag)
        {
          int ByteStartPosition = currStream->frame_bitoffset/8;
          if (currStream->frame_bitoffset % 8 != 0) 
            ByteStartPosition++;
          
          arideco_start_decoding (&currSlice->partArr[2].de_cabac, currStream->streamBuffer, 
            ByteStartPosition, &currStream->read_len, img->type);
        }

        /* LC: resilience code to be inserted */

        FreeNALU(nalu);
        return current_header;

        break;
      case NALU_TYPE_SEI:
        printf ("read_new_slice: Found NALU_TYPE_SEI, len %d\n", nalu->len);
        InterpretSEIMessage(nalu->buf,nalu->len,img);
        break;
      case NALU_TYPE_PPS:
        ProcessPPS(nalu);
        break;

      case NALU_TYPE_SPS:
        ProcessSPS(nalu);
        break;
      case NALU_TYPE_AUD:
//        printf ("read_new_slice: Found 'Access Unit Delimiter' NAL unit, len %d, ignored\n", nalu->len);
        break;
      case NALU_TYPE_EOSEQ:
//        printf ("read_new_slice: Found 'End of Sequence' NAL unit, len %d, ignored\n", nalu->len);
        break;
      case NALU_TYPE_EOSTREAM:
//        printf ("read_new_slice: Found 'End of Stream' NAL unit, len %d, ignored\n", nalu->len);
        break;
      case NALU_TYPE_FILL:
        printf ("read_new_slice: Found NALU_TYPE_FILL, len %d\n", nalu->len);
        printf ("Skipping these filling bits, proceeding w/ next NALU\n");
        break;
      default:
        printf ("Found NALU type %d, len %d undefined, ignore NALU, moving on\n", nalu->nal_unit_type, nalu->len);
    }
  }
  FreeNALU(nalu);

  return  current_header;
}


/*!
 ************************************************************************
 * \brief
 *    Initializes the parameters for a new picture
 ************************************************************************
 */
void init_picture(struct img_par *img, struct inp_par *inp)
{
  int i,k,l;
  Slice *currSlice = img->currentSlice;

  if (dec_picture)
  {
    // this may only happen on slice loss
    exit_picture();
  }

  if (img->frame_num != img->pre_frame_num && img->frame_num != (img->pre_frame_num + 1) % img->MaxFrameNum) 
  {
    if (active_sps->gaps_in_frame_num_value_allowed_flag == 0)
    {
      /* Advanced Error Concealment would be called here to combat unintentional loss of pictures. */
      error("An unintentional loss of pictures occurs! Exit\n", 100);
    }
    fill_frame_num_gap(img);
  }
  img->pre_frame_num = img->frame_num;
  img->num_dec_mb = 0;

  //calculate POC
  decode_poc(img);
  //  dumppoc (img);

  if (img->structure==FRAME ||img->structure==TOP_FIELD)
  {
#ifdef WIN32
    _ftime (&(img->tstruct_start));             // start time ms
#else
    ftime (&(img->tstruct_start));              // start time ms
#endif
    time( &(img->ltime_start));                // start time s
  }

  dec_picture = alloc_storable_picture (img->structure, img->width, img->height, img->width_cr, img->height_cr);
  dec_picture->top_poc=img->toppoc;
  dec_picture->bottom_poc=img->bottompoc;
  dec_picture->frame_poc=img->framepoc;
  dec_picture->qp=img->qp;
  dec_picture->slice_qp_delta=currSlice->slice_qp_delta;
  dec_picture->chroma_qp_offset[0] = active_pps->chroma_qp_index_offset;
  dec_picture->chroma_qp_offset[1] = active_pps->second_chroma_qp_index_offset;

  // reset all variables of the error concealment instance before decoding of every frame.
  // here the third parameter should, if perfectly, be equal to the number of slices per frame.
  // using little value is ok, the code will allocate more memory if the slice number is larger
  ercReset(erc_errorVar, img->PicSizeInMbs, img->PicSizeInMbs, dec_picture->size_x);
  erc_mvperMB = 0;

  switch (img->structure )
  {
  case TOP_FIELD:
    {
      dec_picture->poc=img->toppoc;
      img->number *= 2;
      break;
    }
  case BOTTOM_FIELD:
    {
      dec_picture->poc=img->bottompoc;
      img->number++;
      break;
    }
  case FRAME:
    {
      dec_picture->poc=img->framepoc;
      break;
    }
  default:
    error("img->structure not initialized", 235);
  }
    
  img->current_slice_nr=0;

  if (img->type > SI_SLICE)
  {
    set_ec_flag(SE_PTYPE);
    img->type = P_SLICE;  // concealed element
  }

  // CAVLC init
  for (i=0;i < (int)img->PicSizeInMbs; i++)
    for (k=0;k<4;k++)
      for (l=0;l<(4 + img->num_blk8x8_uv);l++)
        img->nz_coeff[i][k][l]=-1;  // CAVLC

  if(active_pps->constrained_intra_pred_flag)
  {
    for (i=0; i<(int)img->PicSizeInMbs; i++)
    {
      img->intra_block[i] = 1;
    }
  }

  // Set the slice_nr member of each MB to -1, to ensure correct when packet loss occurs
  // TO set Macroblock Map (mark all MBs as 'have to be concealed')
  for(i=0; i<(int)img->PicSizeInMbs; i++)
  {
    img->mb_data[i].slice_nr = -1; 
    img->mb_data[i].ei_flag = 1;
  }

  img->mb_y = img->mb_x = 0;
  img->block_y = img->pix_y = img->pix_c_y = 0; // define vertical positions
  img->block_x = img->pix_x = img->pix_c_x = 0; // define horizontal positions

  dec_picture->slice_type = img->type;
  dec_picture->used_for_reference = (img->nal_reference_idc != 0);
  dec_picture->idr_flag = img->idr_flag;
  dec_picture->no_output_of_prior_pics_flag = img->no_output_of_prior_pics_flag;
  dec_picture->long_term_reference_flag = img->long_term_reference_flag;
  dec_picture->adaptive_ref_pic_buffering_flag = img->adaptive_ref_pic_buffering_flag;

  dec_picture->dec_ref_pic_marking_buffer = img->dec_ref_pic_marking_buffer;
  img->dec_ref_pic_marking_buffer = NULL;

  dec_picture->MbaffFrameFlag = img->MbaffFrameFlag;
  dec_picture->PicWidthInMbs = img->PicWidthInMbs;
  dec_picture->pic_num = img->frame_num;
  dec_picture->frame_num = img->frame_num;
  dec_picture->coded_frame = (img->structure==FRAME);

  dec_picture->chroma_format_idc = active_sps->chroma_format_idc;

  dec_picture->frame_mbs_only_flag = active_sps->frame_mbs_only_flag;
  dec_picture->frame_cropping_flag = active_sps->frame_cropping_flag;

  if (dec_picture->frame_cropping_flag)
  {
    dec_picture->frame_cropping_rect_left_offset   = active_sps->frame_cropping_rect_left_offset;
    dec_picture->frame_cropping_rect_right_offset  = active_sps->frame_cropping_rect_right_offset;
    dec_picture->frame_cropping_rect_top_offset    = active_sps->frame_cropping_rect_top_offset;
    dec_picture->frame_cropping_rect_bottom_offset = active_sps->frame_cropping_rect_bottom_offset;
  }

}

/*!
 ************************************************************************
 * \brief
 *    finish decoding of a picture, conceal errors and store it 
 *    into the DPB
 ************************************************************************
 */
void exit_picture()
{
  char yuv_types[4][6]= {"4:0:0","4:2:0","4:2:2","4:4:4"};
  int ercStartMB;
  int ercSegment;
  frame recfr;
  unsigned int i;
  int structure, frame_poc, slice_type, refpic, qp, pic_num, chroma_format_idc;

  int tmp_time;                   // time used by decoding the last frame
  char yuvFormat[10];

  // return if the last picture has already been finished
  if (dec_picture==NULL)
  {
    return;
  }

  //deblocking for frame or field
  DeblockPicture( img, dec_picture );

  if (dec_picture->MbaffFrameFlag)
    MbAffPostProc();

  recfr.yptr = &dec_picture->imgY[0][0];
  if (dec_picture->chroma_format_idc != YUV400)
  {
    recfr.uptr = &dec_picture->imgUV[0][0][0];
    recfr.vptr = &dec_picture->imgUV[1][0][0];
  }

  //! this is always true at the beginning of a picture
  ercStartMB = 0;
  ercSegment = 0;

  //! mark the start of the first segment
  if (!dec_picture->MbaffFrameFlag)
  {
    ercStartSegment(0, ercSegment, 0 , erc_errorVar);
    //! generate the segments according to the macroblock map
    for(i = 1; i<dec_picture->PicSizeInMbs; i++)
    {
      if(img->mb_data[i].ei_flag != img->mb_data[i-1].ei_flag)
      {
        ercStopSegment(i-1, ercSegment, 0, erc_errorVar); //! stop current segment
        
        //! mark current segment as lost or OK
        if(img->mb_data[i-1].ei_flag)
          ercMarkCurrSegmentLost(dec_picture->size_x, erc_errorVar);
        else
          ercMarkCurrSegmentOK(dec_picture->size_x, erc_errorVar);
        
        ercSegment++;  //! next segment
        ercStartSegment(i, ercSegment, 0 , erc_errorVar); //! start new segment
        ercStartMB = i;//! save start MB for this segment 
      }
    }
    //! mark end of the last segment
    ercStopSegment(dec_picture->PicSizeInMbs-1, ercSegment, 0, erc_errorVar);
    if(img->mb_data[i-1].ei_flag)
      ercMarkCurrSegmentLost(dec_picture->size_x, erc_errorVar);
    else
      ercMarkCurrSegmentOK(dec_picture->size_x, erc_errorVar);
    
    //! call the right error concealment function depending on the frame type.
    erc_mvperMB /= dec_picture->PicSizeInMbs;
    
    erc_img = img;
    if(dec_picture->slice_type == I_SLICE || dec_picture->slice_type == SI_SLICE) // I-frame
      ercConcealIntraFrame(&recfr, dec_picture->size_x, dec_picture->size_y, erc_errorVar);
    else
      ercConcealInterFrame(&recfr, erc_object_list, dec_picture->size_x, dec_picture->size_y, erc_errorVar, dec_picture->chroma_format_idc);
  }

  if (img->structure == FRAME)         // buffer mgt. for frame mode
    frame_postprocessing(img, input);
  else
    field_postprocessing(img, input);   // reset all interlaced variables

  structure  = dec_picture->structure;
  slice_type = dec_picture->slice_type;
  frame_poc  = dec_picture->frame_poc;
  refpic     = dec_picture->used_for_reference;
  qp         = dec_picture->qp;
  pic_num    = dec_picture->pic_num;

  chroma_format_idc= dec_picture->chroma_format_idc;

  store_picture_in_dpb(dec_picture);
  dec_picture=NULL;

  if (img->last_has_mmco_5)
  {
    img->pre_frame_num = 0;
  }

  if ((structure==FRAME)||structure==BOTTOM_FIELD)
  {
    
#ifdef WIN32
    _ftime (&(img->tstruct_end));             // start time ms
#else
    ftime (&(img->tstruct_end));              // start time ms
#endif
    
    time( &(img->ltime_end));                // start time s

    tmp_time=(img->ltime_end*1000+img->tstruct_end.millitm) - (img->ltime_start*1000+img->tstruct_start.millitm);
    tot_time=tot_time + tmp_time;

    sprintf(yuvFormat,"%s", yuv_types[chroma_format_idc]);
    
    if(slice_type == I_SLICE) // I picture
      fprintf(stdout,"%04d(I)  %8d %5d %5d %7.4f %7.4f %7.4f  %s %5d\n",
      frame_no, frame_poc, pic_num, qp, snr->snr_y, snr->snr_u, snr->snr_v, yuvFormat, tmp_time);
    else if(slice_type == P_SLICE) // P pictures
      fprintf(stdout,"%04d(P)  %8d %5d %5d %7.4f %7.4f %7.4f  %s %5d\n",
      frame_no, frame_poc, pic_num, qp, snr->snr_y, snr->snr_u, snr->snr_v, yuvFormat, tmp_time);
    else if(slice_type == SP_SLICE) // SP pictures
      fprintf(stdout,"%04d(SP) %8d %5d %5d %7.4f %7.4f %7.4f  %s %5d\n",
      frame_no, frame_poc, pic_num, qp, snr->snr_y, snr->snr_u, snr->snr_v, yuvFormat, tmp_time);
    else if (slice_type == SI_SLICE)
      fprintf(stdout,"%04d(SI) %8d %5d %5d %7.4f %7.4f %7.4f  %s %5d\n",
      frame_no, frame_poc, pic_num, qp, snr->snr_y, snr->snr_u, snr->snr_v, yuvFormat, tmp_time);
    else if(refpic) // stored B pictures
      fprintf(stdout,"%04d(RB) %8d %5d %5d %7.4f %7.4f %7.4f  %s %5d\n",
      frame_no, frame_poc, pic_num, qp, snr->snr_y, snr->snr_u, snr->snr_v, yuvFormat, tmp_time);
    else // B pictures
      fprintf(stdout,"%04d(B)  %8d %5d %5d %7.4f %7.4f %7.4f  %s %5d\n",
      frame_no, frame_poc, pic_num, qp, snr->snr_y, snr->snr_u, snr->snr_v, yuvFormat, tmp_time);

    fflush(stdout);

    if(slice_type == I_SLICE || slice_type == SI_SLICE || slice_type == P_SLICE || refpic)   // I or P pictures
      img->number++;
    else
      Bframe_ctr++;    // B pictures
    snr->frame_ctr++;

    g_nFrame++;
  }

  img->current_mb_nr = -4712;   // impossible value for debugging, StW
  img->current_slice_nr = 0;

}

/*!
 ************************************************************************
 * \brief
 *    write the encoding mode and motion vectors of current 
 *    MB to the buffer of the error concealment module.
 ************************************************************************
 */

void ercWriteMBMODEandMV(struct img_par *img,struct inp_par *inp)
{
  extern objectBuffer_t *erc_object_list;
  int i, ii, jj, currMBNum = img->current_mb_nr;
  int mbx = xPosMB(currMBNum,dec_picture->size_x), mby = yPosMB(currMBNum,dec_picture->size_x);
  objectBuffer_t *currRegion, *pRegion;
  Macroblock *currMB = &img->mb_data[currMBNum];
  short***  mv;

  currRegion = erc_object_list + (currMBNum<<2);

  if(img->type != B_SLICE) //non-B frame
  {
    for (i=0; i<4; i++)
    {
      pRegion             = currRegion + i;
      pRegion->regionMode = (currMB->mb_type  ==I16MB  ? REGMODE_INTRA      :
                             currMB->b8mode[i]==IBLOCK ? REGMODE_INTRA_8x8  :
                             currMB->b8mode[i]==0      ? REGMODE_INTER_COPY :
                             currMB->b8mode[i]==1      ? REGMODE_INTER_PRED : REGMODE_INTER_PRED_8x8);
      if (currMB->b8mode[i]==0 || currMB->b8mode[i]==IBLOCK)  // INTRA OR COPY
      {
        pRegion->mv[0]    = 0;
        pRegion->mv[1]    = 0;
        pRegion->mv[2]    = 0;
      }
      else
      {
        ii              = 4*mbx + (i%2)*2;// + BLOCK_SIZE;
        jj              = 4*mby + (i/2)*2;
        if (currMB->b8mode[i]>=5 && currMB->b8mode[i]<=7)  // SMALL BLOCKS
        {
          pRegion->mv[0]  = (dec_picture->mv[LIST_0][jj][ii][0] + dec_picture->mv[LIST_0][jj][ii+1][0] + dec_picture->mv[LIST_0][jj+1][ii][0] + dec_picture->mv[LIST_0][jj+1][ii+1][0] + 2)/4;
          pRegion->mv[1]  = (dec_picture->mv[LIST_0][jj][ii][1] + dec_picture->mv[LIST_0][jj][ii+1][1] + dec_picture->mv[LIST_0][jj+1][ii][1] + dec_picture->mv[LIST_0][jj+1][ii+1][1] + 2)/4;
        }
        else // 16x16, 16x8, 8x16, 8x8
        {
          pRegion->mv[0]  = dec_picture->mv[LIST_0][jj][ii][0];
          pRegion->mv[1]  = dec_picture->mv[LIST_0][jj][ii][1];
//          pRegion->mv[0]  = dec_picture->mv[LIST_0][4*mby+(i/2)*2][4*mbx+(i%2)*2+BLOCK_SIZE][0];
//          pRegion->mv[1]  = dec_picture->mv[LIST_0][4*mby+(i/2)*2][4*mbx+(i%2)*2+BLOCK_SIZE][1];
        }
        erc_mvperMB      += mabs(pRegion->mv[0]) + mabs(pRegion->mv[1]);
        pRegion->mv[2]    = dec_picture->ref_idx[LIST_0][jj][ii];
      }
    }
  }
  else  //B-frame
  {
    for (i=0; i<4; i++)
    {
      ii                  = 4*mbx + (i%2)*2;// + BLOCK_SIZE;
      jj                  = 4*mby + (i/2)*2;
      pRegion             = currRegion + i;
      pRegion->regionMode = (currMB->mb_type  ==I16MB  ? REGMODE_INTRA      :
                             currMB->b8mode[i]==IBLOCK ? REGMODE_INTRA_8x8  : REGMODE_INTER_PRED_8x8);
      if (currMB->mb_type==I16MB || currMB->b8mode[i]==IBLOCK)  // INTRA
      {
        pRegion->mv[0]    = 0;
        pRegion->mv[1]    = 0;
        pRegion->mv[2]    = 0;
      }
      else
      {
        int idx = (dec_picture->ref_idx[0][jj][ii]<0)?1:0;
//        int idx = (currMB->b8mode[i]==0 && currMB->b8pdir[i]==2 ? LIST_0 : currMB->b8pdir[i]==1 ? LIST_1 : LIST_0);
//        int idx = currMB->b8pdir[i]==0 ? LIST_0 : LIST_1;
        mv                = dec_picture->mv[idx];
        pRegion->mv[0]    = (mv[jj][ii][0] + mv[jj][ii+1][0] + mv[jj+1][ii][0] + mv[jj+1][ii+1][0] + 2)/4;
        pRegion->mv[1]    = (mv[jj][ii][1] + mv[jj][ii+1][1] + mv[jj+1][ii][1] + mv[jj+1][ii+1][1] + 2)/4;
        erc_mvperMB      += mabs(pRegion->mv[0]) + mabs(pRegion->mv[1]);

        pRegion->mv[2]  = (dec_picture->ref_idx[idx][jj][ii]);
/*        
        if (currMB->b8pdir[i]==0 || (currMB->b8pdir[i]==2 && currMB->b8mode[i]!=0)) // forward or bidirect
        {
          pRegion->mv[2]  = (dec_picture->ref_idx[LIST_0][jj][ii]);
          ///???? is it right, not only "img->fw_refFrArr[jj][ii-4]"
        }
        else
        {
          pRegion->mv[2]  = (dec_picture->ref_idx[LIST_1][jj][ii]);
//          pRegion->mv[2]  = 0;
        }
        */
      }
    }
  }
}

/*!
 ************************************************************************
 * \brief
 *    set defaults for old_slice
 *    NAL unit of a picture"
 ************************************************************************
 */
void init_old_slice()
{
  old_slice.field_pic_flag = 0;

  old_slice.pps_id = INT_MAX;

  old_slice.frame_num = INT_MAX;

  old_slice.nal_ref_idc = INT_MAX;
  
  old_slice.idr_flag = 0;

  old_slice.pic_oder_cnt_lsb          = UINT_MAX;
  old_slice.delta_pic_oder_cnt_bottom = INT_MAX;

  old_slice.delta_pic_order_cnt[0] = INT_MAX;
  old_slice.delta_pic_order_cnt[1] = INT_MAX;

}

/*!
 ************************************************************************
 * \brief
 *    save slice parameters that are needed for checking of "first VCL
 *    NAL unit of a picture"
 ************************************************************************
 */
void exit_slice()
{

  old_slice.pps_id = img->currentSlice->pic_parameter_set_id;

  old_slice.frame_num = img->frame_num;

  old_slice.field_pic_flag = img->field_pic_flag;

  if(img->field_pic_flag)
  {
    old_slice.bottom_field_flag = img->bottom_field_flag;
  }

  old_slice.nal_ref_idc   = img->nal_reference_idc;
  
  old_slice.idr_flag = img->idr_flag;
  if (img->idr_flag)
  {
    old_slice.idr_pic_id = img->idr_pic_id;
  }

  if (active_sps->pic_order_cnt_type == 0)
  {
    old_slice.pic_oder_cnt_lsb          = img->pic_order_cnt_lsb;
    old_slice.delta_pic_oder_cnt_bottom = img->delta_pic_order_cnt_bottom;
  }

  if (active_sps->pic_order_cnt_type == 1)
  {
    old_slice.delta_pic_order_cnt[0] = img->delta_pic_order_cnt[0];
    old_slice.delta_pic_order_cnt[1] = img->delta_pic_order_cnt[1];
  }
}

/*!
 ************************************************************************
 * \brief
 *    detect if current slice is "first VCL NAL unit of a picture"
 ************************************************************************
 */
int is_new_picture()
{
  int result=0;

  result |= (old_slice.pps_id != img->currentSlice->pic_parameter_set_id);

  result |= (old_slice.frame_num != img->frame_num);

  result |= (old_slice.field_pic_flag != img->field_pic_flag);

  if(img->field_pic_flag && old_slice.field_pic_flag)
  {
    result |= (old_slice.bottom_field_flag != img->bottom_field_flag);
  }

  result |= (old_slice.nal_ref_idc   != img->nal_reference_idc);
  
  result |= ( old_slice.idr_flag != img->idr_flag);

  if (img->idr_flag && old_slice.idr_flag)
  {
    result |= (old_slice.idr_pic_id != img->idr_pic_id);
  }

  if (active_sps->pic_order_cnt_type == 0)
  {
    result |=  (old_slice.pic_oder_cnt_lsb          != img->pic_order_cnt_lsb);
    result |=  (old_slice.delta_pic_oder_cnt_bottom != img->delta_pic_order_cnt_bottom);
  }

  if (active_sps->pic_order_cnt_type == 1)
  {
    result |= (old_slice.delta_pic_order_cnt[0] != img->delta_pic_order_cnt[0]);
    result |= (old_slice.delta_pic_order_cnt[1] != img->delta_pic_order_cnt[1]);
  }

  return result;
}


/*!
 ************************************************************************
 * \brief
 *    decodes one slice
 ************************************************************************
 */
void decode_one_slice(struct img_par *img,struct inp_par *inp)
{
  Boolean end_of_slice = FALSE;
  int read_flag;
  img->cod_counter=-1;

  set_ref_pic_num();

  if (img->type == B_SLICE)
      compute_colocated(Co_located, listX);

  //reset_ec_flags();

  while (end_of_slice == FALSE) // loop over macroblocks
  {

#if TRACE
  fprintf(p_trace,"\n*********** POC: %i (I/P) MB: %i Slice: %i Type %d **********\n", img->ThisPOC, img->current_mb_nr, img->current_slice_nr, img->type);
#endif

	// Initializes the current macroblock
    start_macroblock(img,inp, img->current_mb_nr);
    // Get the syntax elements from the NAL
    read_flag = read_one_macroblock(img,inp);
    decode_one_macroblock(img,inp);

    if(img->MbaffFrameFlag && dec_picture->mb_field[img->current_mb_nr])
    {
      img->num_ref_idx_l0_active >>= 1;
      img->num_ref_idx_l1_active >>= 1;
    }

    ercWriteMBMODEandMV(img,inp);

    end_of_slice=exit_macroblock(img,inp,(!img->MbaffFrameFlag||img->current_mb_nr%2));
  }

  exit_slice();
  //reset_ec_flags();
}


void decode_slice(struct img_par *img,struct inp_par *inp, int current_header)
{
  Slice *currSlice = img->currentSlice;

  if (active_pps->entropy_coding_mode_flag)
  {
    init_contexts (img);
    cabac_new_slice();
  }

  if ( (active_pps->weighted_bipred_idc > 0  && (img->type == B_SLICE)) || (active_pps->weighted_pred_flag && img->type !=I_SLICE))
    fill_wp_params(img);

  //printf("frame picture %d %d %d\n",img->structure,img->ThisPOC,img->direct_spatial_mv_pred_flag);
  

  // decode main slice information
  if ((current_header == SOP || current_header == SOS) && currSlice->ei_flag == 0)
    decode_one_slice(img,inp);
    
  // setMB-Nr in case this slice was lost
//  if(currSlice->ei_flag)  
//    img->current_mb_nr = currSlice->last_mb_nr + 1;

}


/*!
 ************************************************************************
 * \brief
 *    Prepare field and frame buffer after frame decoding
 ************************************************************************
 */
void frame_postprocessing(struct img_par *img, struct inp_par *inp)
{
}

/*!
 ************************************************************************
 * \brief
 *    Prepare field and frame buffer after field decoding
 ************************************************************************
 */
void field_postprocessing(struct img_par *img, struct inp_par *inp)
{
  img->number /= 2;
}



void reset_wp_params(struct img_par *img)
{
  int i,comp;
  int log_weight_denom;

  for (i=0; i<MAX_REFERENCE_PICTURES; i++)
  {
    for (comp=0; comp<3; comp++)
    {
      log_weight_denom = (comp == 0) ? img->luma_log2_weight_denom : img->chroma_log2_weight_denom;
      img->wp_weight[0][i][comp] = 1<<log_weight_denom;
      img->wp_weight[1][i][comp] = 1<<log_weight_denom;
    }
  }
}


void fill_wp_params(struct img_par *img)
{
  int i, j, k;
  int comp;
  int log_weight_denom;
  int tb, td;
  int bframe = (img->type==B_SLICE);
  int max_bwd_ref, max_fwd_ref;
  int tx,DistScaleFactor;

  max_fwd_ref = img->num_ref_idx_l0_active;
  max_bwd_ref = img->num_ref_idx_l1_active;

  if (active_pps->weighted_bipred_idc == 2 && bframe)
  {
    img->luma_log2_weight_denom = 5;
    img->chroma_log2_weight_denom = 5;
    img->wp_round_luma = 16;
    img->wp_round_chroma = 16;

    for (i=0; i<MAX_REFERENCE_PICTURES; i++)
    {
      for (comp=0; comp<3; comp++)
      {
        log_weight_denom = (comp == 0) ? img->luma_log2_weight_denom : img->chroma_log2_weight_denom;
        img->wp_weight[0][i][comp] = 1<<log_weight_denom;
        img->wp_weight[1][i][comp] = 1<<log_weight_denom;
        img->wp_offset[0][i][comp] = 0;
        img->wp_offset[1][i][comp] = 0;
      }
    }
  }

  if (bframe)
  {
    for (i=0; i<max_fwd_ref; i++)
    {
      for (j=0; j<max_bwd_ref; j++)
      {
        for (comp = 0; comp<3; comp++)
        {
          log_weight_denom = (comp == 0) ? img->luma_log2_weight_denom : img->chroma_log2_weight_denom;
          if (active_pps->weighted_bipred_idc == 1)
          {
            img->wbp_weight[0][i][j][comp] =  img->wp_weight[0][i][comp];
            img->wbp_weight[1][i][j][comp] =  img->wp_weight[1][j][comp];
          }
          else if (active_pps->weighted_bipred_idc == 2)
          {
            td = Clip3(-128,127,listX[LIST_1][j]->poc - listX[LIST_0][i]->poc);
            if (td == 0 || listX[LIST_1][j]->is_long_term || listX[LIST_0][i]->is_long_term)
            {
              img->wbp_weight[0][i][j][comp] =   32;
              img->wbp_weight[1][i][j][comp] =   32;
            }
            else
            {
              tb = Clip3(-128,127,img->ThisPOC - listX[LIST_0][i]->poc);

              tx = (16384 + abs(td/2))/td;
              DistScaleFactor = Clip3(-1024, 1023, (tx*tb + 32 )>>6);
              
              img->wbp_weight[1][i][j][comp] = DistScaleFactor >> 2;
              img->wbp_weight[0][i][j][comp] = 64 - img->wbp_weight[1][i][j][comp];
              if (img->wbp_weight[1][i][j][comp] < -64 || img->wbp_weight[1][i][j][comp] > 128)
              {
                img->wbp_weight[0][i][j][comp] = 32;
                img->wbp_weight[1][i][j][comp] = 32;
                img->wp_offset[0][i][comp] = 0;
                img->wp_offset[1][i][comp] = 0;
              }
            }
          }
        }
     }
   }
 }

  if (bframe && img->MbaffFrameFlag)
  {
    for (i=0; i<2*max_fwd_ref; i++)
    {
      for (j=0; j<2*max_bwd_ref; j++)
      {
        for (comp = 0; comp<3; comp++)
        {
          for (k=2; k<6; k+=2)
          {
            img->wp_offset[k+0][i][comp] = img->wp_offset[0][i/2][comp];
            img->wp_offset[k+1][i][comp] = img->wp_offset[1][i/2][comp];

            log_weight_denom = (comp == 0) ? img->luma_log2_weight_denom : img->chroma_log2_weight_denom;
            if (active_pps->weighted_bipred_idc == 1)
            {
              img->wbp_weight[k+0][i][j][comp] =  img->wp_weight[0][i/2][comp];
              img->wbp_weight[k+1][i][j][comp] =  img->wp_weight[1][j/2][comp];
            }
            else if (active_pps->weighted_bipred_idc == 2)
            {
              td = Clip3(-128,127,listX[k+LIST_1][j]->poc - listX[k+LIST_0][i]->poc);
              if (td == 0 || listX[k+LIST_1][j]->is_long_term || listX[k+LIST_0][i]->is_long_term)
              {
                img->wbp_weight[k+0][i][j][comp] =   32;
                img->wbp_weight[k+1][i][j][comp] =   32;
              }
              else
              {
                tb = Clip3(-128,127,((k==2)?img->toppoc:img->bottompoc) - listX[k+LIST_0][i]->poc);
                
                tx = (16384 + abs(td/2))/td;
                DistScaleFactor = Clip3(-1024, 1023, (tx*tb + 32 )>>6);

                img->wbp_weight[k+1][i][j][comp] = DistScaleFactor >> 2;
                img->wbp_weight[k+0][i][j][comp] = 64 - img->wbp_weight[k+1][i][j][comp];
                if (img->wbp_weight[k+1][i][j][comp] < -64 || img->wbp_weight[k+1][i][j][comp] > 128)
                {
                  img->wbp_weight[k+1][i][j][comp] = 32;
                  img->wbp_weight[k+0][i][j][comp] = 32;
                  img->wp_offset[k+0][i][comp] = 0;
                  img->wp_offset[k+1][i][comp] = 0;
                }
              }
            }
          }
        }
      }
    }
  }
}
