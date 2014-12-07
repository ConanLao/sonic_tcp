#
# Address Map for Altera 10G MAC + CUSTOM PHY design
#
# For clock synchronization
#
# Support System ID:
#  -- 20150001
# =========================================================

# =================================
# System ID
# =================================
set SYSID_BASE_ADDR      0x00070000
set CLOCKSYNC_ADDR       0x00070020
set LOG_ADDR             0x00070800
# =================================
# Port 0
# =================================
set MAC_BASE_ADDR 	     0x00000000
set LB_BASE_ADDR         0x00010200
set RX_SC_FIFO_BASE_ADDR 0x00010400
set TX_SC_FIFO_BASE_ADDR 0x00010600
set PKT_GEN_BASE_ADDR    0x00020000
set PKT_MON_BASE_ADDR    0x00020040
# =================================
# Port 1
# =================================
set MAC_BASE_ADDR2 		    	0x00040000
set LB_BASE_ADDR2 		    	0x00050200
set RX_SC_FIFO_BASE_ADDR2 	0x00050400
set TX_SC_FIFO_BASE_ADDR2 	0x00050600
set PKT_GEN_BASE_ADDR2      0x00060000
set PKT_MON_BASE_ADDR2      0x00060040
# =================================
# Port 2
# =================================
set MAC_BASE_ADDR3 		    	0x00080000
set LB_BASE_ADDR3 		    	0x00090200
set RX_SC_FIFO_BASE_ADDR3 	0x00090400
set TX_SC_FIFO_BASE_ADDR3 	0x00090600
set PKT_GEN_BASE_ADDR3      0x000a0000
set PKT_MON_BASE_ADDR3      0x000a0040
# =================================
# Port 3
# =================================
set MAC_BASE_ADDR4 		    	0x000c0000
set LB_BASE_ADDR4 		    	0x000d0200
set RX_SC_FIFO_BASE_ADDR4 	0x000d0400
set TX_SC_FIFO_BASE_ADDR4 	0x000d0600
set PKT_GEN_BASE_ADDR4      0x000e0000
set PKT_MON_BASE_ADDR4      0x000e0040

# ==========================================================
#                 MAC address map
# NOTE: currently supported v13.0
# Reference: 10Gbps_MAC.pdf v13.0
# ==========================================================
# RX path
set RX_BACKPRESSURE_BASE_ADDR 	0x00000000
set CRC_PAD_REMOVER_BASE_ADDR 	0x00000100
set CRC_CHECKER_BASE_ADDR 	    0x00000200 
set RX_FRAME_DECODER_BASE_ADDR 	0x00002000
set OVERFLOW_CTRL_BASE_ADDR 		0x00000300
set RX_STATISTICS_BASE_ADDR 		0x00003000

# TX path
set TX_BACKPRESSURE_BASE_ADDR 	0x00004000
set PAD_INSERTER_BASE_ADDR 	    0x00004100
set CRC_INSERTER_BASE_ADDR 	    0x00004200
set PAUSE_GEN_CTRL_BASE_ADDR 		0x00004500
set ADDRESS_INSERTER_BASE_ADDR 	0x00004800
set TX_FRAME_DECODER_BASE_ADDR 	0x00006000
set UNDERFLOW_CTRL_BASE_ADDR 		0x00004300
set TX_STATISTICS_BASE_ADDR 		0x00007000

# =========================================================
# PMA address map
# =========================================================
set MDIO_BASE_ADDR		        0x00001000
set PMA_BASE_ADDR		        	0x00010000

set ALT_PMA_CONTROLLER_BASE_ADDR    0x00000080
set ALT_PMA_RESET_BASE_ADDR  	    	0x00000100
set ALT_PMA_CH_CONTROLLER_BASE_ADDR 0x00000180
