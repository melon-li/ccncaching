#!/usr/bin/python
#coding:utf-8
'''
Created on 2016.6.23

@author: Melon Li
'''


import cityhash

workload_file = "./workload_globetraff"

i=0
dram_dict = {}
files_list = []
pkt_size = 0
collision_cnt = 0
with open(workload_file, 'r') as wkf:
    while(True):
        buf = wkf.readline()
        if(buf == ""):
            break
        pkt = buf.split(" ")
        pkt_size = pkt_size + int(pkt[1])
        files_list.append(buf)

print "Total interest packets of receivers =             %d" % (pkt_size)
print len(files_list)
total = 0
pkt_size = 0
files_set = set(files_list)
while(len(files_set)):
#    print "len = %d" % len(files_set)
    buf = files_set.pop()
    pkt = buf.split(" ")
    pkt_size = pkt_size + int(pkt[1])
print "Total independent interest packets of receivers = %d" % (pkt_size)
