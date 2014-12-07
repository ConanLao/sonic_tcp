#
# Scripts for controlling MAC 
#
# Author: Han Wang
#
# Support Qsys version:
#   -- v10.1
#   -- v11.0
#   -- v11.1
# =========================================================
source address_map.tcl

# Read System ID
proc read_sys_id {} {
    global SYSID_BASE_ADDR
    puts "SysID : [rd32 $SYSID_BASE_ADDR 0x0 0x0]\n"
}

proc clksync { op } {
	global CLOCKSYNC_ADDR
	open_jtag
	if { $op == "enable" } {
		set old [rd32 $CLOCKSYNC_ADDR 0x0 0x0]
		puts 0x$old
		set new [ expr $old & 0xEEEE ]
		puts [format "reg:0x%x to 0x%x" $old $new]
		wr32 $CLOCKSYNC_ADDR 0x0 0x0 $new
	} elseif { $op == "disable" } {
		set old [rd32 $CLOCKSYNC_ADDR 0x0 0x0]
		set new [ expr $old | 0x1111 ]
		puts [format "reg:0x%x to 0x%x" $old $new]
		wr32 $CLOCKSYNC_ADDR 0x0 0x0 $new
	}	else {
		puts "unknown command $op"
	}
	close_jtag
}

proc bypass { op } {
	global CLOCKSYNC_ADDR
	open_jtag
	if { $op == "disable" } {
		set old [rd32 $CLOCKSYNC_ADDR 0x0 0x0]
		set new [ expr $old & 0xDDDD ]
		puts [format "reg:0x%x to 0x%x" $old $new]
		wr32 $CLOCKSYNC_ADDR 0x0 0x0 $new
	} elseif { $op == "enable" } {
		set old [rd32 $CLOCKSYNC_ADDR 0x0 0x0]
		set new [ expr $old | 0x2222 ]
		puts [format "reg:0x%x to 0x%x" $old $new]
		wr32 $CLOCKSYNC_ADDR 0x0 0x0 $new
	}	else {
		puts "unknown command $op"	
  }
	close_jtag
}

#================================================
# Set Logging Base Address: log <portN> <address>
#================================================
proc log { port value } {
	global LOG_ADDR
	open_jtag

	set port_offset [ expr $port << 6 ]
	set write [ expr 1 << 2 ]

	puts "set port$port to $value"
	wr32 $LOG_ADDR $port_offset $write $value
	
	close_jtag
}

#===============================================
#   Clocksync control status
#===============================================
proc status { } {
	global CLOCKSYNC_ADDR
	open_jtag
	set data [rd32 $CLOCKSYNC_ADDR 0x0 0x0]

	puts "clksync control regs: 0x[format %x $data]"
	if { [ expr $data & 0x1111 ] != 0 } {
		puts "clksync disabled"
	} else {
		puts "clksync enabled"
	} 
	
	if { [ expr $data & 0x2222 ] != 0 } {
		puts "clksync bypassed"
	} else {
		puts "clksync not bypassed"
	}
	
	global LOG_ADDR
	for { set i 0 } { $i < 4 } {incr i} {
		set port_offset [ expr $i << 6 ]
		set addr [rd32 $LOG_ADDR $port_offset 0]
		puts "Port $i is writing to 0x[format %x $addr]"
	}
	close_jtag
}


#================================================================
#                       APIs for loopback features              
#================================================================

# Read line loopback register
proc read_loopback {} {
    global LB_BASE_ADDR
    global LB_BASE_ADDR2
    global LB_BASE_ADDR3
    global LB_BASE_ADDR4
    puts "Reading Line Loopback 1: [rd32 $LB_BASE_ADDR 0x0 0x0]\n"
    puts "Reading Local Loopback 1: [rd32 $LB_BASE_ADDR 0x8 0x0]\n"
    puts "Reading Line Loopback 2: [rd32 $LB_BASE_ADDR2 0x0 0x0]\n"
    puts "Reading Local Loopback 2: [rd32 $LB_BASE_ADDR2 0x8 0x0]\n"
    puts "Reading Line Loopback 3: [rd32 $LB_BASE_ADDR3 0x0 0x0]\n"
    puts "Reading Local Loopback 3: [rd32 $LB_BASE_ADDR3 0x8 0x0]\n"
    puts "Reading Line Loopback 4: [rd32 $LB_BASE_ADDR4 0x0 0x0]\n"
    puts "Reading Local Loopback 4: [rd32 $LB_BASE_ADDR4 0x8 0x0]\n"
}

# Write to line loopback back register with the given value
# value = 0 or 1
proc write_line_loopback {value port} {
    global LB_BASE_ADDR
    global LB_BASE_ADDR2
    global LB_BASE_ADDR3
    global LB_BASE_ADDR4

    if {$port == 0} {
        set lb_base_addr $LB_BASE_ADDR
    } elseif {$port == 1} {
        set lb_base_addr $LB_BASE_ADDR2
    } elseif {$port == 2} {
        set lb_base_addr $LB_BASE_ADDR3
    } elseif {$port == 3} {
        set lb_base_addr $LB_BASE_ADDR4
    }

    puts "Writing $value to Line Loopback\n"
    wr32 $lb_base_addr 0x0 0x0 $value
}

proc write_local_loopback {value port} {
    global LB_BASE_ADDR
    global LB_BASE_ADDR2
    global LB_BASE_ADDR3
    global LB_BASE_ADDR4

    if {$port == 0} {
        set lb_base_addr $LB_BASE_ADDR
    } elseif {$port == 1} {
        set lb_base_addr $LB_BASE_ADDR2
    } elseif {$port == 2} {
        set lb_base_addr $LB_BASE_ADDR3
    } elseif {$port == 3} {
        set lb_base_addr $LB_BASE_ADDR4
    }

    puts "Writing $value to Local Loopback\n"
    wr32 $lb_base_addr 0x8 0x0 $value
}

#==============================================================================
#                       APIs for RX FIFO              
#==============================================================================
proc set_fifo_drop_on_error {value port} {
    global RX_SC_FIFO_BASE_ADDR
    global RX_SC_FIFO_BASE_ADDR2
    global RX_SC_FIFO_BASE_ADDR3
    global RX_SC_FIFO_BASE_ADDR4

    if {$port == 0} {
        set rx_addr $RX_SC_FIFO_BASE_ADDR
    } elseif {$port == 1} {
        set rx_addr $RX_SC_FIFO_BASE_ADDR2
    } elseif {$port == 2} {
        set rx_addr $RX_SC_FIFO_BASE_ADDR3
    } elseif {$port == 3} {
        set rx_addr $RX_SC_FIFO_BASE_ADDR4
    }

    puts "Writing $value to drop on error register of RX FIFO\n"
    wr32 $rx_addr 0x14 0x0 $value
}


#==============================================================================
#                       APIs for MAC              
#==============================================================================
# Read all registers of address inserter
proc read_address_inserter {port} {
    global MAC_BASE_ADDR
    global MAC_BASE_ADDR2
    global MAC_BASE_ADDR3
    global MAC_BASE_ADDR4
    global ADDRESS_INSERTER_BASE_ADDR
    global add_inserter

    if {$port == 0} {
        set mac_address $MAC_BASE_ADDR
    } elseif {$port == 1} {
        set mac_address $MAC_BASE_ADDR2
    } elseif {$port == 2} {
        set mac_address $MAC_BASE_ADDR3
    } elseif {$port == 3} {
        set mac_address $MAC_BASE_ADDR4
    }

    puts "=============================================================================="
    puts "                      Reading Address Inserter CSR			        "
    puts "==============================================================================\n\n"
    puts "control status : [rd32 $mac_address $ADDRESS_INSERTER_BASE_ADDR 0x0]\n"
    puts "MAC1 src addr 0 : [rd32 $mac_address $ADDRESS_INSERTER_BASE_ADDR 0x4]\n"
    puts "MAC1 src addr 1 : [rd32 $mac_address $ADDRESS_INSERTER_BASE_ADDR 0x8]\n"
}

# Write to MAC source address register with the given value
proc write_MAC_src_address {value port} {
    global MAC_BASE_ADDR
    global MAC_BASE_ADDR2
    global MAC_BASE_ADDR3
    global MAC_BASE_ADDR4
    global ADDRESS_INSERTER_BASE_ADDR

    if {$port == 0} {
        set mac_address $MAC_BASE_ADDR
    } elseif {$port == 1} {
        set mac_address $MAC_BASE_ADDR2
    } elseif {$port == 2} {
        set mac_address $MAC_BASE_ADDR3
    } elseif {$port == 3} {
        set mac_address $MAC_BASE_ADDR4
    }


    puts "=============================================================================="
    puts "                      Write MAC Source Address to Address Inserter 		"
    puts "==============================================================================\n\n"
    set offset 0x4
    set lowerMAC 0x[string range $value 6 13]
    puts "Write to offset $offset of Address Inserter with value of $lowerMAC\n"
    wr32 $mac_address $ADDRESS_INSERTER_BASE_ADDR $offset $lowerMAC

    set offset 0x8
    set higherMAC [string range $value 0 5]
    puts "Write to offset $offset of Address Inserter with value of $higherMAC\n"
    wr32 $mac_address $ADDRESS_INSERTER_BASE_ADDR $offset $higherMAC
}

proc set_address_inserter {value port} {
    global MAC_BASE_ADDR
    global MAC_BASE_ADDR2
    global MAC_BASE_ADDR3
    global MAC_BASE_ADDR4
    global ADDRESS_INSERTER_BASE_ADDR
    set offset 0x0

    if {$port == 0} {
        set mac_address $MAC_BASE_ADDR
    } elseif {$port == 1} {
        set mac_address $MAC_BASE_ADDR2
    } elseif {$port == 2} {
        set mac_address $MAC_BASE_ADDR3
    } elseif {$port == 3} {
        set mac_address $MAC_BASE_ADDR4
    }


    puts "=============================================================================="
    puts "                      Write to Control register of Address Inserter 1 	        "
    puts "==============================================================================\n\n"
    puts "Write to offset $offset of Address inserter with value of $value\n"
    wr32 $mac_address $ADDRESS_INSERTER_BASE_ADDR $offset $value
}

proc read_mac_control {port} {
    global MAC_BASE_ADDR
    global MAC_BASE_ADDR2
    global MAC_BASE_ADDR3
    global MAC_BASE_ADDR4
    
    if {$port == 0} {
        set mac_address $MAC_BASE_ADDR
    } elseif {$port == 1} {
        set mac_address $MAC_BASE_ADDR2
    } elseif {$port == 2} {
        set mac_address $MAC_BASE_ADDR3
    } elseif {$port == 3} {
        set mac_address $MAC_BASE_ADDR4
    }


    set base_address 0x0
    set read_error 0x0

    puts "=============================================================================="
    puts "                      Read MAC control register	  	            "
    puts "==============================================================================\n\n"
    
    puts "rx_transfer_control: [rd32 $mac_address $base_address 0x00]\n"	
    puts "rx_transfer_status: [rd32 $mac_address $base_address 0x04]\n"	
    puts "rx_padcrc_control: [rd32 $mac_address $base_address 0x10]\n"	
    puts "rx_padcrc_control: [rd32 $mac_address $base_address 0x10]\n"	
    puts "rx_crccheck_control: [rd32 $mac_address $base_address 0x20]\n"	
    puts "rx_frame_control: [rd32 $mac_address $base_address 0x100]\n"	
    puts "rx_frame_maxlen: [rd32 $mac_address $base_address 0x104]\n"	
    puts "rx_frame_addr0: [rd32 $mac_address $base_address 0x108]\n"	
    puts "rx_frame_addr1: [rd32 $mac_address $base_address 0x10C]\n"	

}

# Read all statistics registers of the given path (rx or tx)
proc read_stats {path file_path log port} {
    global MAC_BASE_ADDR
    global MAC_BASE_ADDR2
    global MAC_BASE_ADDR3
    global MAC_BASE_ADDR4
    global TX_STATISTICS_BASE_ADDR
    global RX_STATISTICS_BASE_ADDR
    
    if {$port == 0} {
        set mac_address $MAC_BASE_ADDR
    } elseif {$port == 1} {
        set mac_address $MAC_BASE_ADDR2
    } elseif {$port == 2} {
        set mac_address $MAC_BASE_ADDR3
    } elseif {$port == 3} {
        set mac_address $MAC_BASE_ADDR4
    }

    set base_address 0x0
    set read_error 0x0
    
    if {$path ==  "tx"} {
        puts "=============================================================================="
        puts "                      Reading TX Statistics			            "
        puts "==============================================================================\n\n"
        set base_address $TX_STATISTICS_BASE_ADDR
    } elseif {$path == "rx"} {
        puts "=============================================================================="
        puts "                      Reading RX Statistics			            "
        puts "==============================================================================\n\n"
        set base_address $RX_STATISTICS_BASE_ADDR
    } else {
        puts "Wrong argument for read_stats\n"
        set read_error 0x1
    }

    if {$read_error == 0x0} {
        puts "clr : [rd32 $mac_address $base_address 0x00]\n"
        puts "framesOK : [rd64 $mac_address $base_address 0x08]\n"
        puts "framesErr : [rd64 $mac_address $base_address 0x10]\n"
        puts "framesCRCErr : [rd64 $mac_address $base_address 0x18]\n"
        puts "octetsOK : [rd64 $mac_address $base_address 0x20]\n"
        puts "pauseMACCtrlFrames : [rd64 $mac_address $base_address 0x28]\n"
        puts "ifErrors : [rd64 $mac_address $base_address 0x30]\n"
        puts "unicastFramesOK : [rd64 $mac_address $base_address 0x38]\n"
        puts "unicastFramesErr : [rd64 $mac_address $base_address 0x40]\n"
        puts "multicastFramesOK : [rd64 $mac_address $base_address 0x48]\n"
        puts "multicastFramesErr : [rd64 $mac_address $base_address 0x50]\n"
        puts "broadcastFramesOK : [rd64 $mac_address $base_address 0x58]\n"
        puts "broadcastFramesErr : [rd64 $mac_address $base_address 0x60]\n"
        puts "etherStatsOctets : [rd64 $mac_address $base_address 0x68]\n"
        puts "etherStatsPkts : [rd64 $mac_address $base_address 0x70]\n"
        puts "etherStatsUnderSizePkts : [rd64 $mac_address $base_address 0x78]\n"
        puts "etherStatsOversizePkts : [rd64 $mac_address $base_address 0x80]\n"
        puts "etherStatsPkts64Octets : [rd64 $mac_address $base_address 0x88]\n"
        puts "etherStatsPkts65to127Octets : [rd64 $mac_address $base_address 0x90]\n"
        puts "etherStatsPkts128to255Octets : [rd64 $mac_address $base_address 0x98]\n"
        puts "etherStatsPkts256to511Octet : [rd64 $mac_address $base_address 0xA0]\n"
        puts "etherStatsPkts512to1023Octets : [rd64 $mac_address $base_address 0xA8]\n"
        puts "etherStatsPkts1024to1518Octets : [rd64 $mac_address $base_address 0xB0]\n"
        puts "etherStatsPkts1518toXOctets : [rd64 $mac_address $base_address 0xB8]\n"
        puts "etherStatsFragments : [rd64 $mac_address $base_address 0xC0]\n"
        puts "etherStatsJabbers : [rd64 $mac_address $base_address 0xC8]\n"
        puts "etherStatsCRCErr : [rd64 $mac_address $base_address 0xD0]\n"
        puts "unicastMACCtrlFrames : [rd64 $mac_address $base_address 0xD8]\n"
        puts "multicastMACCtrlFrames : [rd64 $mac_address $base_address 0xE0]\n"
        puts "broadcastMACCtrlFrames : [rd64 $mac_address $base_address 0xE8]\n" 
    }

    if {$log == "1"} {
        set chan [open $file_path a+]
        puts $chan "clr,[rd32 $mac_address $base_address 0x00]"
        puts $chan "framesOK,[rd64 $mac_address $base_address 0x08]"
        puts $chan "framesErr,[rd64 $mac_address $base_address 0x10]"
        puts $chan "framesCRCErr,[rd64 $mac_address $base_address 0x18]"
        puts $chan "octetsOK,[rd64 $mac_address $base_address 0x20]"
        puts $chan "pauseMACCtrlFrames,[rd64 $mac_address $base_address 0x28]"
        puts $chan "ifErrors,[rd64 $mac_address $base_address 0x30]"
        puts $chan "unicastFramesOK,[rd64 $mac_address $base_address 0x38]"
        puts $chan "unicastFramesErr,[rd64 $mac_address $base_address 0x40]"
        puts $chan "multicastFramesOK,[rd64 $mac_address $base_address 0x48]"
        puts $chan "multicastFramesErr,[rd64 $mac_address $base_address 0x50]"
        puts $chan "broadcastFramesOK,[rd64 $mac_address $base_address 0x58]"
        puts $chan "broadcastFramesErr,[rd64 $mac_address $base_address 0x60]"
        puts $chan "etherStatsOctets,[rd64 $mac_address $base_address 0x68]"
        puts $chan "etherStatsPkts,[rd64 $mac_address $base_address 0x70]"
        puts $chan "etherStatsUnderSizePkts,[rd64 $mac_address $base_address 0x78]"
        puts $chan "etherStatsOversizePkts,[rd64 $mac_address $base_address 0x80]"
        puts $chan "etherStatsPkts64Octets,[rd64 $mac_address $base_address 0x88]"
        puts $chan "etherStatsPkts65to127Octets,[rd64 $mac_address $base_address 0x90]"
        puts $chan "etherStatsPkts128to255Octets,[rd64 $mac_address $base_address 0x98]"
        puts $chan "etherStatsPkts256to511Octet,[rd64 $mac_address $base_address 0xA0]"
        puts $chan "etherStatsPkts512to1023Octets,[rd64 $mac_address $base_address 0xA8]"
        puts $chan "etherStatsPkts1024to1518Octets,[rd64 $mac_address $base_address 0xB0]"
        puts $chan "etherStatsPkts1518toXOctets,[rd64 $mac_address $base_address 0xB8]"
        puts $chan "etherStatsFragments,[rd64 $mac_address $base_address 0xC0]"
        puts $chan "etherStatsJabbers,[rd64 $mac_address $base_address 0xC8]"
        puts $chan "etherStatsCRCErr,[rd64 $mac_address $base_address 0xD0]"
        puts $chan "unicastMACCtrlFrames,[rd64 $mac_address $base_address 0xD8]"
        puts $chan "multicastMACCtrlFrames,[rd64 $mac_address $base_address 0xE0]"
        puts $chan "broadcastMACCtrlFrames,[rd64 $mac_address $base_address 0xE8]" 
        close $chan
    }
}

#  Configure Packet Generator 
proc config_pkt_gen { cnt_pkt sz_pkt port } {
    global PKT_GEN_BASE_ADDR
    global PKT_GEN_BASE_ADDR2
    global PKT_GEN_BASE_ADDR3
    global PKT_GEN_BASE_ADDR4

    if {$port == 0} {
        set pkt_gen_base_addr $PKT_GEN_BASE_ADDR
    } elseif {$port == 1} {
        set pkt_gen_base_addr $PKT_GEN_BASE_ADDR2
    } elseif {$port == 2} {
        set pkt_gen_base_addr $PKT_GEN_BASE_ADDR3
    } elseif {$port == 3} {
        set pkt_gen_base_addr $PKT_GEN_BASE_ADDR4
    }

    puts "=============================================================================="
    puts "                      Write to Control register of Packet Generator  	        "
    puts "==============================================================================\n\n"
    
    puts "Write Seed A, B, C"
    wr32 $pkt_gen_base_addr 0x30 0x0 0x8912C983
    wr32 $pkt_gen_base_addr 0x34 0x0 0x3B97DA93
    wr32 $pkt_gen_base_addr 0x38 0x0 0x091E5894

    puts "Write Number of Packet to transmit $cnt_pkt"
    wr32 $pkt_gen_base_addr 0x0 0x0 $cnt_pkt
    
    set val [expr ($sz_pkt << 1)]
#    set val [set_bit $val 15]
    set val [clear_bit $val 15]
    wr32 $pkt_gen_base_addr 0x4 0x0 $val
    puts "Config (packet size, payload type)"
    puts "       ([expr ($sz_pkt)], pattern_sel = [expr (($val & 0x8000) >> 15)], 0=incremental payload, 1=random payload)"

    puts "Source Address"
    wr32 $pkt_gen_base_addr 0x10 0x0 0x00C28001
    wr32 $pkt_gen_base_addr 0x14 0x0 0x00002010

    puts "Dest Address"
    wr32 $pkt_gen_base_addr 0x18 0x0 0x00C28002
    wr32 $pkt_gen_base_addr 0x1C 0x0 0x00002010

}

proc get_tx_count {port} {
    global PKT_GEN_BASE_ADDR
    global PKT_GEN_BASE_ADDR2
    global PKT_GEN_BASE_ADDR3
    global PKT_GEN_BASE_ADDR4

    if {$port == 0} {
        set pkt_gen_base_addr $PKT_GEN_BASE_ADDR
    } elseif {$port == 1} {
        set pkt_gen_base_addr $PKT_GEN_BASE_ADDR2
    } elseif {$port == 2} {
        set pkt_gen_base_addr $PKT_GEN_BASE_ADDR3
    } elseif {$port == 3} {
        set pkt_gen_base_addr $PKT_GEN_BASE_ADDR4
    }

    open_jtag
    puts "\n - Pkt Gen Operation : [rd32 $pkt_gen_base_addr 0x8 0x0]"
    puts "\n - Destination MAC 0 : [rd32 $pkt_gen_base_addr 0x18 0x0]"
    puts "\n - Destination MAC 1 : [rd32 $pkt_gen_base_addr 0x1C 0x0]"
    puts "\n - Number of Generated Packets : [rd32 $pkt_gen_base_addr 0x24 0x0]"
    close_jtag
}

# configure packet monitor
proc config_pkt_mon {port} {
    global PKT_MON_BASE_ADDR
    global PKT_MON_BASE_ADDR2
    global PKT_MON_BASE_ADDR3
    global PKT_MON_BASE_ADDR4
    
    if {$port == 0} {
        set pkt_mon_base_addr $PKT_MON_BASE_ADDR
    } elseif {$port == 1} {
        set pkt_mon_base_addr $PKT_MON_BASE_ADDR2
    } elseif {$port == 2} {
        set pkt_mon_base_addr $PKT_MON_BASE_ADDR3
    } elseif {$port == 3} {
        set pkt_mon_base_addr $PKT_MON_BASE_ADDR4
    }



    puts "=============================================================================="
    puts "                      Write to Control register of Packet Monitor  	        "
    puts "==============================================================================\n\n"

    puts "Expected RX count"
    wr32 $pkt_mon_base_addr 0x0 0x0 0xFF
}

proc pkt_mon_test {port} {

    global PKT_MON_BASE_ADDR
    global PKT_MON_BASE_ADDR2
    global PKT_MON_BASE_ADDR3
    global PKT_MON_BASE_ADDR4

    if {$port == 0} {
        set pkt_mon_base_addr $PKT_MON_BASE_ADDR
    } elseif {$port == 1} {
        set pkt_mon_base_addr $PKT_MON_BASE_ADDR2
    } elseif {$port == 2} {
        set pkt_mon_base_addr $PKT_MON_BASE_ADDR3
    } elseif {$port == 3} {
        set pkt_mon_base_addr $PKT_MON_BASE_ADDR4
    }



    puts "=============================================================================="
    puts "                      Start pakcet generator and monitor  	        "
    puts "==============================================================================\n\n"

    puts "\n - Number of Received Correct Frames : [rd32 $pkt_mon_base_addr 0x4 0x0]"
    puts "\n - Number of Received Frames with Error: [rd32 $pkt_mon_base_addr 0x8 0x0]"
    puts "\n - number of packet: [rd32 $pkt_mon_base_addr 0x00 0x0]"
    puts "\n - packet rx ok: [rd32 $pkt_mon_base_addr 0x04 0x0]"
    puts "\n - packet rx error: [rd32 $pkt_mon_base_addr 0x08 0x0]"
    puts "\n - byte rx count0: [rd32 $pkt_mon_base_addr 0x0C 0x0]"
    puts "\n - byte rx count1: [rd32 $pkt_mon_base_addr 0x10 0x0]"
    puts "\n - cycle rx count0: [rd32 $pkt_mon_base_addr 0x14 0x0]"
    puts "\n - cycle rx count1: [rd32 $pkt_mon_base_addr 0x18 0x0]"
    puts "\n - Received status: [rd32 $pkt_mon_base_addr 0x1C 0x0]"
}

#=================================================
#  Single Port Loopback on Port 0
#=================================================
proc start_10gbaser_loopback_test {} {
    global PKT_MON_BASE_ADDR
    global PKT_GEN_BASE_ADDR

    puts "=============================================================================="
    puts "                      Start pakcet generator and monitor  	        "
    puts "==============================================================================\n\n"

    # Set both port to serial loopback.
    set_fpga_pma_loopback 3 

    wr32 $PKT_MON_BASE_ADDR 0x1C 0x0 1
    wr32 $PKT_GEN_BASE_ADDR 0x08 0x0 1

    puts "Polling Packet Monitor Status until done or error ..." 
    puts "\n This will take some time ... \n"
    
    set regvalue [rd32 $PKT_MON_BASE_ADDR 0x1C 0x0]

    #	while {$regvalue&0x04 == 0} {
    #		after 100
    #		set regvalue [rd32 $PKT_MON_BASE_ADDR 0x1C 0x0]
    #	}

    after 2000	

    puts "\n - Number of Transmitted Correct Frames : [rd32 $PKT_GEN_BASE_ADDR 0x24 0x0]"
    puts "\n - Number of Received Correct Frames : [rd32 $PKT_MON_BASE_ADDR 0x4 0x0]"
    puts "\n - Number of Received Frames with Error: [rd32 $PKT_MON_BASE_ADDR 0x8 0x0]"
    puts "\n - Received status: [rd32 $PKT_MON_BASE_ADDR 0x1C 0x0]"
    
    pkt_mon_test 0
    set_fpga_pma_loopback 0
    after 100
}

proc config_mac {port} {
    open_jtag

    puts "=============================================================================="
    puts "                      Accessing Ethernet 10G MAC CSR			    "
    puts "==============================================================================\n\n"

    #==============================================================================
    #                       Configuring RX fifo to                
    #============================================================================
    set_fifo_drop_on_error 1 $port

    #==============================================================================
    #                       Configuring MAC Source Address               
    #==============================================================================
    read_address_inserter $port
    set_address_inserter 1 $port
    # Note: that the write MAC src address must be little endian!
    # The proper src MAC address is 20-10-00-C2-80-01 for Port 0
    # The proper src MAC address is 20-10-00-C2-80-02 for Port 1
    #
    if { $port == 0 } {
        write_MAC_src_address 0x0180C2001020 $port
    } elseif { $port == 1} {
        write_MAC_src_address 0x0280C2001020 $port
    } elseif { $port == 2} {
        write_MAC_src_address 0x0380C2001020 $port
    } elseif { $port == 3} {
        write_MAC_src_address 0x0480C2001020 $port
    }

    read_address_inserter $port

    close_jtag
}

proc run_mac_test {port} {

    open_jtag

    puts "=============================================================================="
    puts "                      Start Ethernet 10G packet loopback test			    "
    puts "==============================================================================\n\n"


    # External PHY Status and Control Signals
    # Bit 11-8: 0, SFP2_MOD, SFP1_MOD, SFP1_TX_FAULT
    # Bit 7-4: SFP2_TX_FAULT, L2_PWRDN, L1_PWRDN, S2_TX_DISABLE
    # Bit 3-0: S1_TX_DISABLE, SFP2_RSEL, SFP1_RSEL, PHY_RESET_N
    read_stats tx "" 0 $port
    read_stats rx "" 0 $port
    read_mac_control

    #set_pma_loopback

    config_pkt_gen 0xFFFF 69 $port
    config_pkt_mon $port

    start_10gbaser_loopback_test

    read_stats tx "" 0 $port
    read_stats rx "" 0 $port

    close_jtag

}

proc log_mac { logfile port } {
    open_jtag

    puts "Mac address:"
    read_address_inserter $port

    puts "Mac monitor stats:"
    pkt_mon_test $port

    puts "Mac internal counters:"
    read_stats rx $logfile 1 $port

    close_jtag
}

proc stats_mac { port } {
    open_jtag

    puts "Mac address:"
    read_address_inserter $port

    puts "Mac monitor stats:"
    pkt_mon_test $port

    puts "Mac internal counters:"
    read_stats tx "" 0 $port
    read_stats rx "" 0 $port

    close_jtag
}

proc clr_mac {} {
    global RX_STATISTICS_BASE_ADDR
    global MAC_BASE_ADDR
    global MAC_BASE_ADDR2
    global MAC_BASE_ADDR3
    global MAC_BASE_ADDR4

    open_jtag

    wr32 $MAC_BASE_ADDR $RX_STATISTICS_BASE_ADDR 0x0 1
    wr32 $MAC_BASE_ADDR2 $RX_STATISTICS_BASE_ADDR 0x0 1
    wr32 $MAC_BASE_ADDR3 $RX_STATISTICS_BASE_ADDR 0x0 1
    wr32 $MAC_BASE_ADDR4 $RX_STATISTICS_BASE_ADDR 0x0 1
    puts "Clear all Rx statistics."

    close_jtag
}

proc gen_mac { len size port } {
    global PKT_GEN_BASE_ADDR
    
    open_jtag

    config_pkt_gen $len $size $port
    wr32 $PKT_GEN_BASE_ADDR 0x08 0x0 1
    
    close_jtag
}

