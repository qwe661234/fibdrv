make
sudo rmmod fibdrv
sudo insmod fibdrv.ko
sudo taskset -c 11 ./client 
