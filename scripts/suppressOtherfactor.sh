sudo sh -c "echo 0 > /proc/sys/kernel/randomize_va_space"
sudo sh performance.sh
sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"