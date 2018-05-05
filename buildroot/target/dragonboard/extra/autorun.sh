#!/bin/sh

source script_parser.sh

if [ ! -d /system/vendor/ ]; then
    mkdir -p /system/vendor/
    ln -s /lib/modules/`uname -r`/ /system/vendor/modules

if [ ! -d /system/etc/ ]; then
    mkdir -p /system/etc/
    ln -s /dragonboard/bin/hawkview /system/etc/hawkview
fi
    ln -s /boot/*.hcd /system/vendor/modules/
    ln -s /boot/*.bin /system/vendor/modules/
    ln -s /boot/*.txt /system/vendor/modules/
fi

#Before insmod disp.ko need to insmod sunxi_tr.ko first
insmod /lib/modules/`uname -r`/sunxi_tr.ko
if [ $? -ne 0 ]; then
	echo "insmod /lib/modules/`uname -r`/sunxi_tr.ko failed"
fi

insmod /lib/modules/`uname -r`/disp.ko
if [ $? -ne 0 ]; then
	echo "insmod /lib/modules/`uname -r`/disp.ko failed"
fi

insmod /lib/modules/`uname -r`/lcd.ko
if [ $? -ne 0 ]; then
	echo "insmod /lib/modules/`uname -r`/lcd.ko failed"
fi

ROOT_DEVICE="/dev/mmcblk0p7"
for parm in $(cat /proc/cmdline); do
    case $parm in
        root=*)
            ROOT_DEVICE=`echo $parm | awk -F\= '{print $2}'`
            ;;
    esac
done

# install nand driver if we boot from sdmmc
nand_activated=`script_fetch "nand" "activated"`
echo "nand activated #$nand_activated"
if [ $nand_activated -eq 1 ]; then
    case $ROOT_DEVICE in
        /dev/mmc*)
      
        nand_module_path=`script_fetch "nand" "module_path"`
        if [ -n "$nand_module_path" ]; then
            insmod "$nand_module_path"
       fi
            ;;
    esac

fi
# insmod touchscreen driver
tp_module_path=`script_fetch "tp" "module_path"`
event_num=`script_fetch "tp" "event_num"`
if [ -n "$tp_module_path" ]; then
    insmod "$tp_module_path"
	 while true; do
		if [ ! -c /dev/input/event$event_num ]; then
			echo "Can not find /dev/input/event$event_num"
			sleep 1
			continue
		else 
			echo "Found /dev/input/event$event_num"
			# calibrate touchscreen if need
			tp_type=`script_fetch "tp" "type"`
			if [ $tp_type -eq 0 ]; then
				while true; do
					ts_calibrate
					if [ $? -eq 0 ]; then
						break
					fi
				done
			fi
			break
		fi
	done 
else
    echo "NO!!! touchscreen driver to be insmod"
fi

#insmod sw-key driver
key_module_path=`script_fetch "key" "module_path"`
insmod "$key_module_path"

# insmod ir driver
ir_activated=`script_fetch "ir" "activated"`
if [ $ir_activated -eq 1 ]; then
    ir_module_path=`script_fetch "ir" "module_path"`
    if [ -n "$ir_module_path" ]; then
        insmod "$ir_module_path"
    fi
fi

# start camera test firstly
while true; do
    camera_activated=`script_fetch "camera" "activated"`
    echo "camera activated #$camera_activated"
    if [ $camera_activated -eq 1 ]; then
        echo "camera activated"
        module_count=`script_fetch "camera" "module_count"`
        if [ $module_count -gt 0 ]; then
            for i in $(seq $module_count); do
                key_name="module"$i"_path"
                module_path=`script_fetch "camera" "$key_name"`
                if [ -n "$module_path" ]; then
                    insmod "$module_path"
                    if [ $? -ne 0 ]; then
                        echo "insmod $module_path failed"
                        break 2
                    fi
                fi
            done
        fi
    else
        echo "camera not activated"
        break
    fi

    echo "camera module insmod done"
    touch /tmp/camera_insmod_done
done

# fix some driver download firmware from /system/vendor/modules.
# android style
if [ ! -d /system/vendor/ ]; then
    mkdir -p /system/vendor/
    ln -s /lib/modules/3.4.39/ /system/vendor/modules
fi

if [ ! -d /data/misc/dmt/ ]; then
    mkdir -p /data/misc/dmt/
fi

# run dragonboard core process
core &
