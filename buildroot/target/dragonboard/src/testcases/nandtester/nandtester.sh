#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh

ROOT_DEVICE=/dev/nandd
for parm in $(cat /proc/cmdline); do
    case $parm in
        root=*)
            ROOT_DEVICE=`echo $parm | awk -F\= '{print $2}'`
            ;;
    esac
done

case $ROOT_DEVICE in
    /dev/nand*)
        echo "nand boot"
        mount /dev/nanda /boot
        SEND_CMD_PIPE_OK_EX $3
        exit 1
        ;;
    /dev/mmc*)
        echo "mmc boot"
        mount /dev/mmcblk0p2 /boot
        ;;
    *)
        echo "default boot type"
        mount /dev/nanda /boot
        ;;
esac

#only in card boot mode,it will run here 
echo "nand test ioctl start"
nandrw "/dev/nanda" 

if [ $? -ne 0 ]; then
    SEND_CMD_PIPE_FAIL_EX $3 "nand ioctl failed"
    exit 1
else
	echo "nand ok"
	SEND_CMD_PIPE_OK_EX $3 
fi
