#!/bin/bash

CPUID=11
# ORIG_ASLR=`cat /proc/sys/kernel/randomize_va_space`
# ORIG_GOV=`cat /sys/devices/system/cpu/cpu$CPUID/cpufreq/scaling_governor`
# ORIG_TURBO=`cat /sys/devices/system/cpu/intel_pstate/no_turbo`

sudo bash -c "echo 0 > /proc/sys/kernel/randomize_va_space"
sudo bash -c "echo performance > /sys/devices/system/cpu/cpu$CPUID/cpufreq/scaling_governor"
sudo bash -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"

#!/bin/bash
for file in `find /proc/irq -name "smp_affinity"`
do
    # var=0x`cat ${file}`
    # var="$(( $var & 0x7ff ))"
    # var=`printf '%.3x' ${var}`
    # echo ${var}
    sudo bash -c "echo 7ff > ${file}"
done
sudo bash -c "echo 7ff > /proc/irq/default_smp_affinity"

# make
# sudo rmmod fibdrv
# sudo insmod fibdrv.ko
# sudo taskset -c 11 ./client

# restore the original system settings
# sudo bash -c "echo $ORIG_ASLR >  /proc/sys/kernel/randomize_va_space"
# sudo bash -c "echo $ORIG_GOV > /sys/devices/system/cpu/cpu$CPUID/cpufreq/scaling_governor"
# sudo bash -c "echo $ORIG_TURBO > /sys/devices/system/cpu/intel_pstate/no_turbo"

# itr=0
# for file in `find /proc/irq -name "smp_affinity"`
# do
#     var=${arr[${itr}]}
#     var=`printf '%.2x' ${var}`
#     sudo bash -c "echo ${var} > ${file}"
#     (( itr++ ))
# done
# sudo bash -c "echo fff > /proc/irq/default_smp_affinity"