/*
 * random_variable.h
 *
 *  Created on: Dec 24, 2013
 *      Author: tsilochr
 */

#ifndef EXPERIMENT_GLOBALS_H
#define EXPERIMENT_GLOBALS_H

#define LRU_RATE  10 //Gbps
#define LRU_ENTRY_SIZE 40
#define OPC_ENTRY_SIZE 42

//optimized_reading_cache_size = reading_cache_size*OPT_RATI
#define OPT_RATIO   9/16

//ENABLE_AGGREGATION: true enable; false disable.
#define ENABLE_AGGREGATION true

#define CCN_PROTO 0x88DD
#define NUMBER_OF_RUNS 1

#define ROOT_DOMAIN "D1"
#define ZIPF_A 0.75

#define WIN_MAX 7000 //7000
#define LINK_DELAY "5ms"
#define ACCESS_LINK_DELAY "5ms"
#define TTL 0.005*4 //second
/*
The PointToPointNetDevice models a transmitter section that puts bits on a corresponding channel “wire.” The DataRate attribute specifies the number of bits per second that the device will simulate sending over the channel. In reality no bits are sent, but an event is scheduled for an elapsed time consistent with the number of bits in each packet and the specified DataRate. The implication here is that the receiving device models a receiver section that can receive any any data rate. Therefore there is no need, nor way to set a receive data rate in this model. By setting the DataRate on the transmitter of both devices connected to a given PointToPointChannel one can model a symmetric channel; or by setting different DataRates one can model an asymmetric channel.
*/
#define LINK_CAPACITY "100Gbps"
#define ACCESS_LINK_CAPACITY  "100Gbps"// LINK_CAPACITY/50, just for send rate, receiving rate is unlimited
#define LINK_THROUGHTPUT "93Gbps" // 1Gbps
#define USER_EXPERIENCED_RATE "1Gbps"

#define PAYLOAD_SIZE 10 // The actual data packet is not saved, but its used for compatibility..
#define REQ_SIZE 30

#define CACHE_CAPACITY 100 
#define CACHE_MODE "packet" // "packet" or "object"
#define PACKET_CACHE_MODE 1  //LRU
#define OBJECT_CACHE_MODE 2  //OPC
#define SRAM_CACHE_MODE 3  //sram cache for dram, certainly file index is also stored in sram 

//sram cache parameters
#define WIDTH 8 //bytes
#define PKT_SIZE 1500
#define PKT_NUM 8
#define FILE_NUM 8 //4
#define DRAM_SIZE 100 //GB

//#define CACHE_PLACEMENT 0 // 0 for edges, 1 for betweenness and 2 for all nodes

#define CACHING_START_INDEX 1
#define MAX_LOG_CHUNK_ID 1000

#define WORKLOAD_FILE "./src/ccncaching/files/workload_globetraff" //workload path

#define NODES_TYPE 1 //access nodes or core nodes


#define SRAM_ACCESS_TIME 450 // pico seconds
#define DRAM_ACCESS_TIME 30000 // pico seconds
#define DRAM_OLD_ACCESS_TIME 625 //pico seconds


#include "ns3/core-module.h"

namespace ns3 {

class ExperimentGlobals{
public:
    static Ptr<UniformRandomVariable> RANDOM_VAR;
    static uint8_t CACHE_PLACEMENT;

};

}  // namespace ns3
#endif /* EXPERIMENT_GLOBALS_H */
