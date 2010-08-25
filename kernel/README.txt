Directory:
        cd ~/kernel/

Load/unload:
        insmod qrandom.ko
        rmmod qrandom.ko

To add entropy:
        echo 1111 > /dev/qrandom

To get randomness back:  
        cat /dev/qrandom

To see diagnostics:  
        dmesg   


