/*
 * random_variable.h
 *
 *  Created on: Dec 24, 2013
 *      Author: tsilochr
 */

#ifndef EXPERIMENT_GLOBALS_H
#define EXPERIMENT_GLOBALS_H

#define CCN_PROTO 0x88DD
#define NUMBER_OF_RUNS 1

#define ROOT_DOMAIN "domain1"
#define ZIPF_A 0.75

#define WIN_MAX 8
#define LINK_DELAY "1ms"
#define ACCESS_LINK_DELAY "1ms"
#define LINK_CAPACITY "100000Mbps"
#define ACCESS_LINK_CAPACITY  "100000"// "10Mbps"

#define PAYLOAD_SIZE 10 // The actual data packet is not saved, but its used for compatibility..

#define CACHE_CAPACITY 100 
#define CACHE_MODE "packet" // "packet" or "object"
#define PACKET_CACHE_MODE 1  //LRU
#define OBJECT_CACHE_MODE 2  //OPC
#define SRAM_CACHE_MODE 3  //sram cache for dram, certainly file index is also stored in sram 

//sram cache parameters
#define WIDTH 8 //bytes
#define PKT_SIZE 1500
#define PKT_NUM 8
#define FILE_NUM 4
#define DRAM_SIZE 100 //GB

//#define CACHE_PLACEMENT 0 // 0 for edges, 1 for betweenness and 2 for all nodes

#define CACHING_START_INDEX 1
#define MAX_LOG_CHUNK_ID 1000

#define WORKLOAD_FILE "./src/ccncaching/files/workload_globetraff" //workload path

#define NODES_TYPE 1 //access nodes or core nodes


#define SRAM_ACCESS_TIME 450 // in pico seconds
#define DRAM_ACCESS_TIME 30000 // in pico seconds
#define DRAM_OLD_ACCESS_TIME 625


#include "ns3/core-module.h"

namespace ns3 {

class ExperimentGlobals{
public:
    static Ptr<UniformRandomVariable> RANDOM_VAR;
    static uint8_t CACHE_PLACEMENT;

};

}  // namespace ns3
#endif /* EXPERIMENT_GLOBALS_H */
