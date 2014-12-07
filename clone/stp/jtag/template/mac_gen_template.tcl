#
# Generate parameterized packet streams from MAC core.
# Parameters:
# pkt_cnt: total count of packets to generate
# pkt_len: length of each packet payload.
#
source lib_common.tcl
#source liconfig.tcl
source lib_mac.tcl

gen_mac cnt_str len_str 0
