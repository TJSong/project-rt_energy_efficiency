#include <sys/time.h>
#include <stdio.h>

#ifndef __TIMING_H__
#define __TIMING_H__

struct timeval start, end;

void start_timing();
void end_timing();
void print_timing();

/*
 * Start of section to time.
 */
void start_timing() {
  gettimeofday(&start, NULL);
}

/*
 * Stop timing, record end time.
 */
void end_timing() {
  gettimeofday(&end, NULL);
}

/*
 * Print timing information to stdout.
 */
void print_timing() {
  static int instance_number = 0;
  printf("time %d = %d us\n", instance_number, 
    (int)(end.tv_sec - start.tv_sec)*1000000 + (int)(end.tv_usec - start.tv_usec));
  instance_number++;
}

/*
 * Print timing information to stdout.
 */
void print_set_dvfs_timing() {
  static int instance_number = 0;
  printf("time_set_dvfs %d = %d us\n", instance_number, 
    (int)(end.tv_sec - start.tv_sec)*1000000 + (int)(end.tv_usec - start.tv_usec));
  instance_number++;
}

/*
 * Print timing information to stdout.
 */
int exec_timing() {
//  static int instance_number = 0;
//  printf("time %d = %d us\n", instance_number, 
  return (int)(end.tv_sec - start.tv_sec)*1000000 + (int)(end.tv_usec - start.tv_usec);
}


/*
 * Write timing information out to a file.
 */
void write_timing() {
  static int instance_number = 0;

  // Open file for writing
  FILE *time_file;
  if (instance_number == 0) {
    time_file = fopen("times.txt", "w");
  } else {
    time_file = fopen("times.txt", "a");
  }
  if (time_file == NULL) {
    printf("Error opening times.txt!\n");
    exit(1);
  }

  // Write time
  int time = (int)(end.tv_sec - start.tv_sec)*1000000 + (int)(end.tv_usec - start.tv_usec);
  fprintf(time_file, "time %d = %d us\n", instance_number, time);
  instance_number++;

  fclose(time_file);
}

/*
 * Write the passed integer array to the timing file.
 */
void write_array(int *array, int array_len) {
  static int instance_number = 0;

  // Open file for writing
  FILE *time_file;
  if (instance_number == 0) {
    time_file = fopen("times.txt", "w");
  } else {
    time_file = fopen("times.txt", "a");
  }
  if (time_file == NULL) {
    printf("Error opening times.txt!\n");
    exit(1);
  }

  // Write time
  fprintf(time_file, "loop counter = (");
  int i;
  for (i = 0; i < array_len; i++) {
      fprintf(time_file, "%d, ", array[i]++);
  }
  fprintf(time_file, ")\n");
  instance_number++;

  fclose(time_file);
}

#endif
