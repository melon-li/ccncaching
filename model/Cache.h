
/*
 * Cache.h
 *
 *  This is a copy of the code that is used in the real implementation of a packet-cache module for blackadder PSI prototype which operates over click modular router.
 *  However in this version all writting and reading operations that concern the ACTUAL transfered data (not cache management) are stubs.
 *   
 *  Created on: Oct 21, 2013
 *      Author: Y.T.
 */

#ifndef CACHE_H_
#define CACHE_H_

#include <string>
#include <bf.h>

#include "ns3/core-module.h"
#include "ns3/CcnModule.h"
#include "ns3/experiment_globals.h"


namespace ns3 {

class LRU_Object{
public:
	LRU_Object(){}
	LRU_Object(const string & _filename){
		filename = _filename;
		stored_packets = 0;
		prev = NULL;
		next = NULL;
		//click_chatter("Created new LRU_Object %s", _filename.c_str());
		}

	string filename;
	LRU_Object * prev;
	LRU_Object * next;
	uint32_t stored_packets;
};

class LRU_Table{
public:
	LRU_Table(){
		stored_files = 0;
		}
		
	uint32_t stored_files;
	LRU_Object* head, *tail;
	map<string, LRU_Object*> objects; 
	
	void add_object(LRU_Object* obj);
	void remove_object(const LRU_Object* obj);
	void update_object(LRU_Object* obj, bool new_object = false);
        void test();
};

class CacheModule: public Object{
public:
	CacheModule(uint32_t _capacity,uint32_t _capacity_fast_table){
		capacity = _capacity;
		capacity_fast_table = _capacity_fast_table;
		stored_packets=0;
		stored_files=0;
		LRU = new LRU_Table();
		requests=0;
		hits=0;		
		responses=0;  
		log_chunk_id_hits = new uint32_t[MAX_LOG_CHUNK_ID];
		std::fill(log_chunk_id_hits, log_chunk_id_hits + MAX_LOG_CHUNK_ID, 0);
		reads_for_insertions =0 ;
		reads_for_fetchings =0 ;
		reads_for_evictions =0 ;
	}

	~CacheModule(){
		//delete LRU;
	}
	
	//these used to be pure virtual but ns3 could not handle it
    virtual uint32_t add_packet(const string& _filename, const string& ID, const char* _payload, const bool is_first_packet){return 0;}
    virtual uint32_t remove_last_packet(const string& _filename){return 0;}
	virtual uint32_t get_cached_packet(const string& _filename, const string& ID){return 0;}
    virtual uint32_t cache_packet(const string& _filename, const string& ID, const char* _payload){return 0;}
	virtual string get_state(){return 0;}
	virtual string get_packet_stats(){return 0;}
	
	uint32_t stored_files;
    LRU_Table *LRU;
    uint32_t capacity; // in packets
    uint32_t capacity_fast_table; // fast index table cap
    uint32_t stored_packets;
    uint32_t reads_for_insertions;
    uint32_t reads_for_fetchings;
    uint32_t reads_for_evictions;

    map<string , char* > data_table; 
      
    uint64_t requests;
    uint64_t hits;
    uint64_t responses;   
    
    uint32_t *log_chunk_id_hits ;
    map<string, uint32_t>log_file_hits; // this gets erazed when its written
    string get_file_hits();
    string get_chunk_id_hits();
};

class O_Cache: public CacheModule{
public:
	O_Cache(uint32_t _capacity, uint32_t _capacity_fast_table):CacheModule(_capacity,_capacity_fast_table){
		stored_packets = 0;
		stored_files = 0;
		zero_pcks=0;   
		added_pcks=0;   
		}
	
	// <filename, max chunk id>
    map<string , uint32_t> index_table;  
    // <filename, allocated memory (IN PACKETS!)
    map<string , uint32_t> mem_table; 
    // <filename, stats metric> metric may be a successfull hit counter, and will be used for pacekt removals
    map<string , uint32_t> stats_table; 
   
    uint32_t add_packet(const string& _filename, const string& _ID, const char* _payload, const bool is_first_packet);
    uint32_t remove_last_packet(const string& _filename);  
    int32_t remove_last_file();//new by argi
    int32_t get_stored_packets(const string& _filename);//new by argi
    uint32_t get_cached_packet(const string& _filename, const string& _ID);
    uint32_t cache_packet(const string& _filename, const string& _ID, const char* _payload);
    string get_state();
    string get_packet_stats();
		        
    uint64_t zero_pcks;   
    uint64_t added_pcks; 
};

class P_Cache: public CacheModule{
public:
	P_Cache(uint32_t _capacity, uint32_t _capacity_fast_table):CacheModule(_capacity, _capacity_fast_table){}
		
	uint32_t get_cached_packet(const string& _filename, const string& _ID);
    uint32_t cache_packet(const string& _filename, const string& _ID, const char* _payload);
    uint32_t add_packet(const string& _filename, const string& _ID, const char* _payload, const bool is_first_packet);
    uint32_t remove_last_packet(const string& _filename);
    
	string get_state();
    string get_packet_stats(){return "";}
     
};

}
#endif