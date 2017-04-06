#!/usr/bin/python
#coding:utf-8
'''
Created on 2016.6.18

@author: Melon Li
'''


import cityhash

FILE_NUM = 4
PKT_NUM = 8
PKT_SIZE = 1500
DRAM_SIZE = 15*1024*1024*1024 #byte
SLOT_NUM = DRAM_SIZE/PKT_SIZE/PKT_NUM/FILE_NUM
workload_file = "./workload_globetraff"

print "slot_num = %d" %SLOT_NUM
i=0
dram_dict = {}
files_set = set()
# files_set = []
collision_cnt = 0
with open(workload_file, 'r') as wkf:
    while(True):
        buf = wkf.readline()
        if(buf == ""):
            break
        files_set.add(buf)
        # files_set.append(buf)

total = 0
#total = len(files_set)
pkt_size = 0
while(len(files_set)):
#    print "len = %d" % len(files_set)
    buf = files_set.pop()
    pkt = buf.split(" ")
#    print pkt
    pkt_size = pkt_size + int(pkt[1])
    for j in range(0, (int(pkt[1])/PKT_NUM + 1)):
        total = total + 1
        key = "/D1/"+pkt[0]+"-"+ str(j)
        addr = cityhash.CityHash64(key)
        addr = addr%SLOT_NUM
        if(dram_dict.has_key(addr)):
            #files refere to  list
            files = dram_dict[addr];
            if(files.count(key) == 0):
                if(len(files) == FILE_NUM):
                    collision_cnt = collision_cnt + 1
                else:
                    files.append(key)
                   # dram_dict[addr] = files
        else:
            files = []
            files.append(key)
            dram_dict[addr] = files
#    i = i + 1
#    if(i==10):
#        break
print "total=%d, collision_cnt=%d, collision_rate=%.4f (Percent)" %(total, collision_cnt, (100*float(collision_cnt)/total))
print "total size=%.2f GB" % (pkt_size*1500/1024.0/1024/1024)
