#!/bin/tcsh -f

set mplab = "/opt/microchip/mplabx/v3.61/mplab_ide/bin/"

cat << END > /tmp/prog.x
device pic32MX270F256D
set poweroptions.powerenable true
set voltagevalue 3.3
hwtool pickit3 -p
#program "tools/bootloader_2019.hex" 
# has LED
program "tools/bootloader-20200128.hex" 
#no led
#program "tools/bootload_2019_alt.hex" 
quit
END

#program "./bootloader_2015.hex" 
# USB_HID_Btl_StarterKit.X.production.hex
$mplab/mdb.sh /tmp/prog.x

