/*
 * random_variable.h
 *
 *  Created on: Dec 24, 2013
 *      Author: tsilochr
 */

#ifndef EXPERIMENT_GLOBALS_H
#define EXPERIMENT_GLOBALS_H

#define NUMBER_OF_RUNS 1

#define ROOT_DOMAIN "domain1"
#define ZIPF_A 0.75

#define LINK_DELAY "5ms"
#define ACCESS_LINK_DELAY "5ms"
#define LINK_CAPACITY "5Mbps"
#define ACCESS_LINK_CAPACITY "50Mbps"

#define PAYLOAD_SIZE 10 // The actual data packet is not saved, but its used for compatibility..

#define CACHE_CAPACITY 100 
#define CACHE_MODE "packet" // "packet" or "object"
#define PACKET_CACHE_MODE 1
#define OBJECT_CACHE_MODE 2

//#define CACHE_PLACEMENT 0 // 0 for edges, 1 for betweenness and 2 for all nodes

#define CACHING_START_INDEX 1
#define MAX_LOG_CHUNK_ID 1000

#define WORKLOAD_FILE "./src/ccncaching/files/workload_globetraff" //workload path

#define NODES_TYPE 1 //access nodes or core nodes


#define SRAM_ACCESS_TIME 450 // in pico seconds
#define DRAM_ACCESS_TIME 55000 // in pico seconds

#include "ns3/core-module.h"

namespace ns3 {

class ExperimentGlobals{
public:
    static Ptr<UniformRandomVariable> RANDOM_VAR;
    static uint8_t CACHE_PLACEMENT;

};

}  // namespace ns3
#endif /* EXPERIMENT_GLOBALS_H */
