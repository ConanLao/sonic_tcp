#!/bin/sh

QUARTUS_PGM_PATH="/opt/altera/13.0/quartus/bin/quartus_pgm"

cable=1

if [ $1 = "two" ]; then
    firmware=stp_two_ports_switch.sof
elif [ $1 = "four" ]; then
    firmware=stp_four_ports_switch.sof
else
    firmware=PCIe_Fundamental_time_limited.sof

fi

echo "Loading $firmware"

cp $firmware firmware.sof

$QUARTUS_PGM_PATH -c $cable PCIe_Fundamental_time_limited.cdf
