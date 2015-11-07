#!/bin/bash

source global.sh
BENCHMARK_FOLDER=${BENCH_NAME[$1]}
BENCHMARK=$BENCHMARK_FOLDER"-"$4

if [[ $# < 4 ]] ; then
  echo 'USAGE : ./run.sh [bench index] [big/little] [governors] [sweep]'
  exit 1
fi

if [ $2 != "big" ] && [ $2 != "little" ] && [ $2 != "hetero" ] ; then
  echo 'USAGE : ./run.sh [bench index] [big/little] [governors] [sweep]'
  exit 1
fi

if [ $2 == "big" ] ; then
    WHICH_CPU="cpu7"
    TASKSET_FLAG="0x80"
    MAX_FREQ=2000000
    SENSOR_ID="3-0040"
elif [ $2 == "little" ] ; then
	if [ $ARCH_TYPE == "amd64" ] ; then 
    WHICH_CPU="cpu3"
    TASKSET_FLAG="0x08"
    MAX_FREQ=2534000
	elif [ $ARCH_TYPE == "armhf" ] ; then 
    WHICH_CPU="cpu3"
		TASKSET_FLAG="0x08"
		MAX_FREQ=1400000
	else 
		echo "unknown architecture"
		exit 1
	fi
  SENSOR_ID="3-0045"
elif [ $2 == "hetero" ] ; then
  WHICH_CPU="cpu3"
  TASKSET_FLAG="0x08"
  MAX_FREQ=1400000
  SENSOR_ID="3-0045"
fi

init(){
	if [ $ARCH_TYPE == "amd64" ] ; then 
    sudo chmod 777 /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
    sudo chmod 777 /sys/devices/system/cpu/cpu3/cpufreq/scaling_max_freq
    sudo chmod 777 /sys/devices/system/cpu/cpu3/cpufreq/scaling_cur_freq
    echo $MAX_FREQ > /sys/devices/system/cpu/cpu3/cpufreq/scaling_max_freq 
    echo performance > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
  elif [ $ARCH_TYPE == "armhf" ] ; then 
    sudo chmod 777 /sys/devices/system/cpu/cpu7/cpufreq/scaling_governor
    sudo chmod 777 /sys/devices/system/cpu/cpu7/cpufreq/scaling_max_freq
    sudo chmod 777 /sys/bus/i2c/drivers/INA231/3-0040/sensor_W
    sudo chmod 777 /sys/devices/system/cpu/cpu7/cpufreq/scaling_cur_freq
    echo 2000000 > /sys/devices/system/cpu/cpu7/cpufreq/scaling_max_freq 
    echo performance > /sys/devices/system/cpu/cpu7/cpufreq/scaling_governor
    sudo chmod 777 /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
    sudo chmod 777 /sys/devices/system/cpu/cpu3/cpufreq/scaling_max_freq
    sudo chmod 777 /sys/bus/i2c/drivers/INA231/3-0045/sensor_W
    sudo chmod 777 /sys/devices/system/cpu/cpu3/cpufreq/scaling_cur_freq
    echo 1400000 > /sys/devices/system/cpu/cpu3/cpufreq/scaling_max_freq 
    echo performance > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
	else 
		echo "unknown architecture"
		exit 1
	fi
}

init 

if [[ $3 ]] ; then
    mkdir -p $PROJECT_PATH/dvfs_sim/data_odroid/$2/$BENCHMARK_FOLDER/$BENCHMARK
    echo $PROJECT_PATH/dvfs_sim/data_odroid/$2/$BENCHMARK_FOLDER/$BENCHMARK
    echo $3 > /sys/devices/system/cpu/$WHICH_CPU/cpufreq/scaling_governor
    if [[ $5 ]] ; then
		if [ $4 == "freq_sweep" ] ; then
			echo $5 > /sys/devices/system/cpu/$WHICH_CPU/cpufreq/scaling_max_freq 
		elif [ $4 == "freq_other_sweep" ] ; then
    		if [ $2 == "big" ] ; then
				echo 1400000 > /sys/devices/system/cpu/$WHICH_CPU/cpufreq/scaling_max_freq 
				echo $4 > /sys/devices/system/cpu/$WHICH_CPU/cpufreq/scaling_max_freq 
    		elif [ $2 == "little" ] ; then
				echo 1400000 > /sys/devices/system/cpu/$WHICH_CPU/cpufreq/scaling_max_freq 
				echo $4 > /sys/devices/system/cpu/$WHICH_CPU/cpufreq/scaling_max_freq 
			fi
		fi
    fi
    sleep 1;
    echo $3"..."

    PRE_PWD=`pwd`
    cd $PRE_PWD
    echo ${BENCH_NAME[$1]}"..."
    if [ ${BENCH_NAME[$1]} == "sha_preread" ] || \
       [ ${BENCH_NAME[$1]} == "rijndael_preread" ] || \
       [ ${BENCH_NAME[$1]} == "stringsearch" ] || \
       [ ${BENCH_NAME[$1]} == "2048_slice" ] || \
       [ ${BENCH_NAME[$1]} == "curseofwar_slice_sdl" ] || \
       [ ${BENCH_NAME[$1]} == "pocketsphinx" ] ; then
      ./gen_runme_slice.py > runme_slice.sh
      chmod a+x runme_slice.sh
      sleep 3
      cat /sys/devices/system/cpu/$WHICH_CPU/cpufreq/scaling_governor
      taskset $TASKSET_FLAG ./runme_slice.sh
    elif [ ${BENCH_NAME[$1]} == "xpilot_slice" ] ; then
      taskset $TASKSET_FLAG ./src/server/xpilots > output_slice.txt &
      sleep 3;
      taskset $TASKSET_FLAG ./src/client/xpilot &
      PROCESS_CNT_BEFORE=$(pgrep -c 'xpilot')
      #find the window 
      xdotool search --sync --onlyvisible --class "xpilot"  
      #press join
	    if [ $ARCH_TYPE == "amd64" ] ; then 
        xdotool mousemove 1483 314; sleep 3;
	    elif [ $ARCH_TYPE == "armhf" ] ; then 
        #maximize the window
        xdotool key alt+F10;        sleep 3;
        xdotool mousemove 65 95;    sleep 3;
      fi
      xdotool click 1;            sleep 3;
      #press click
	    if [ $ARCH_TYPE == "amd64" ] ; then 
        xdotool mousemove 1861 325; sleep 3;
	    elif [ $ARCH_TYPE == "armhf" ] ; then 
        #maximize the window
        xdotool key alt+F10;        sleep 3;
        xdotool mousemove 490 100;  sleep 3;
      fi
      xdotool click 1;            sleep 3;
      #playing
      while true;
      do
        if [[ $PROCESS_CNT_BEFORE > $(pgrep -c 'xpilot') ]] ; then
          echo "xpilot ends"
          break
        fi
        xdotool keydown Return 
        xdotool keydown shift+a
        sleep 3
        xdotool keyup Return 
        xdotool keyup shift+a
      done
      echo "xdotool done"
      PID_FREECIV_SERVER=$(pgrep 'xpilot')
      kill -9 $PID_FREECIV_SERVER
    fi

#    echo ${BENCH_NAME[$1]}"..."
#    ./gen_runme_slice_dummy.py > runme_slice.sh
#    chmod a+x runme_slice.sh
#    sleep 3
#    taskset $TASKSET_FLAG ./runme_slice.sh &
#    sleep 3;
#
#    PRE_PWD=`pwd`
#    cd /$PROJECT_PATH/dummy/
#    /$PROJECT_PATH/dummy/dummy.sh $2 &
#    /$PROJECT_PATH/dummy/dummy.sh $2 &
#    /$PROJECT_PATH/dummy/dummy.sh $2 &
#    sleep 120; #dummy.sh takes around 150s at lowest freq on little core
#    cd $PRE_PWD 
#
    cp $PRE_PWD/output_slice.txt $PROJECT_PATH/dvfs_sim/data_odroid/$2/$BENCHMARK_FOLDER/$BENCHMARK/$3
else
    echo "specify governor!"
    exit 1
fi

#SET TO PERFORMANCE AFTER RUN ALL
init

echo "[ run.sh "$3" done ]"
exit 0

