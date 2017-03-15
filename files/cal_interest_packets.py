#!/usr/bin/python
#coding:utf-8
'''
Created on 2016.6.23

@author: Melon Li
'''


import cityhash
import sys

if len(sys.argv) != 2:
    print 'Usage:%s <workload_file>' % sys.argv[0]
    sys.exit(1)
workload_file = sys.argv[1]

if workload_file == "":
    workload_file = "./workload_globetraff"

print "Analyze %s" % workload_file

i=0
dram_dict = {}
files_list = []
#files_name = set()
pkt_size = 0
collision_cnt = 0
with open(workload_file, 'r') as wkf:
    while(True):
        buf = wkf.readline()
        buf = buf.strip()
        if(buf == ""):
            break
        pkt = buf.split(" ")
        if len(pkt) < 2: pkt = buf.split("\t")
        pkt_size = pkt_size + int(pkt[1])
       # files_name.add(pkt[0])
        files_list.append(buf)

print "Total interest packets of receivers =             %d, %.2f GB" % ( pkt_size, pkt_size*1500.0/(10**9))
#print len(files_name)
total = 0
old = pkt_size
pkt_size = 0
files_set = set(files_list)
#print len(files_set)
while(len(files_set)):
#    print "len = %d" % len(files_set)
    buf = files_set.pop()
    pkt = buf.split(" ")
    if len(pkt) < 2: pkt = buf.split("\t")
    pkt_size = pkt_size + int(pkt[1])
print "Total independent interest packets of receivers = %d, %.2f GB" % (pkt_size, pkt_size*1500.0/(10**9))
print "Red rate=%.2f" % (pkt_size/float(old))
