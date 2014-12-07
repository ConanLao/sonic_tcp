#!/bin/bash

run_mac_init () {
    system-console -cli --script="mac_scripts/mac_init.tcl"
}

run_mac_collect_stat () {
    system-console -cli --script="mac_scripts/mac_stat.tcl"
}

run_mac_clear_stat () {
    system-console -cli --script="mac_scripts/mac_clear.tcl"
}

run_mac_gen_pkt () {
    system-console -cli --script="mac_scripts/mac_gen.tcl"
}

# Entry
if [ "$1" = "init" ] 
then
    run_mac_init
elif [ "$1" = "clear" ]
then
    run_mac_clear_stat
elif [ "$1" = "capture" ] 
then
    if [ $# -ne 3 ]
    then 
        echo "Error in $0 - Invalid Argument Count"
        echo "capture file_path port_num"
        exit
    fi
    file_path=$2
    port_num=$3
    cat template/mac_stat_template.tcl | sed -e 's/path_str/'$file_path'/g' | sed -e 's/port_num/'$port_num'/g' > mac_scripts/mac_stat.tcl
    run_mac_collect_stat
elif [ "$1" = "gen" ]
then
    if [ $# -ne 4 ]
    then
        echo "Error in $0 - Invalid Argument Count"
        echo "gen pkt_cnt pkt_len"
        exit
    fi
    pkt_cnt=$2
    pkt_len=$3
    cat template/mac_gen_template.tcl | sed -e 's/cnt_str/'$pkt_cnt'/g' | sed -e 's/len_str/'$pkt_len'/g' > mac_scripts/mac_gen.tcl
    run_mac_gen_pkt
else
    echo "usage: mac_automator.sh <cmd> <param>"
    echo "<cmd>: init, run, capture, gen"
    echo "run: no parameter"
    echo "capture: file_path port"
    echo "gen: pkt_cnt pkt_len"
fi

exit 0
