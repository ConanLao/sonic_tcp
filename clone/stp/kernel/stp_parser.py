#! /usr/bin/python
import sys, os, argparse, numpy, math, textwrap

#from sonic import *

verbose=False

def isEqual(e1,e2):
    for i in range(len(e1)):
        if e1[i] != e2[i]:
            return False

    return True

def append_adjustment(f, time, adjust):
	if (f):
		outstr = "{} {}\n".format(time.split('.')[1], adjust.split(',')[1].rstrip())
		f.write(outstr)

def parse_stp_messages(ifname):
    input_file=open(ifname, 'r')
    out1 = open(ifname+".p0",'w')
    out2 = open(ifname+".p1",'w')
    out3 = open('adjust.p0', 'a')
    out4 = open('adjust.p1', 'a')

    mask=0x3fffffffffffffL
    events={}
    debug = 0
    i = 0
    j = 0
    for line in input_file:
        sep=line.split()

        if line[0] == '-':
            continue

        if line[0] == '!':
            if (line[1] == '1'):
                append_adjustment(out4, ifname, line)
            else:
                append_adjustment(out3, ifname, line)
            continue

        latency= (int("0x"+sep[7], 16) >> 24)
        ref = ((int("0x"+sep[7], 16) << 32) + int("0x"+sep[6],16)) & mask
        prev = ((int("0x"+sep[5], 16) << 32) + int("0x"+sep[4],16)) & mask
        prev_state = int("0x"+sep[5],16) >> 29
        cur = ((int("0x"+sep[3], 16) << 32) + int("0x"+sep[2],16)) & mask
        cur_state = int("0x"+sep[3],16) >> 29
        remote= ((int("0x"+sep[1],16) <<32) + int("0x"+sep[0],16)) & mask
        message_type= int("0x"+sep[1],16) >> 22

        # beacon_send
#        if message_type == 0x60:
#            continue

        outstring="{} {} {} {} {} {} {} \n".format(i, hex(ref), latency, hex(remote),  hex(prev), hex(cur), hex(remote + latency - cur))
        x=""
        if j >=64:
            out2.write(outstring)
            x="[p1]"
        else:
            out1.write(outstring)
            x="[p0]"

        if message_type == 0x60:
            outstring="{} beacon_send {} {} {} {} ".format(x, hex(ref), latency, hex(prev), hex(cur))
        else:
            outstring="{} beacon_recv {} {} {} {} {} {} ".format(x, hex(ref), latency, hex(remote),  hex(prev), hex(cur), hex(remote + latency - cur))

        if ref in events:
            events[ref] = outstring + " [duplicate]"
        else:
            events[ref] = outstring

        i+=1
        j+=1

        #print ref, prev, cur, remote

        # states: INIT = 3'b001, SENT = 3'b010, SYNC = 3'b100

#        print i, hex(ref), hex(remote), latency, hex(prev), hex(cur), hex(remote+latency - cur)

#        if i == 6420:
#            break

        if j == 128:
            j = 0 

#        if remote + latency != cur:
#            debug += 1
#            if debug == 10:
#                sys.exit(1);

    out4.close()
    out3.close()
    out2.close()
    out1.close()
    input_file.close()

#    for key in sorted(events):
#        print events[key]


def main():
    global verbose

    parser=argparse.ArgumentParser();
    parser.add_argument('-v', '--verbose', action='store_true', default=False)
#    parser.add_argument('-l', '--latency', type=int, required=True)
    parser.add_argument('-i', '--input', type=str, 
            help='Input file', required=True)

    args=parser.parse_args()

    if args.verbose:
        verbose = True

    if verbose:
        print args

    parse_stp_messages(args.input)

if __name__ == "__main__":
    main()

