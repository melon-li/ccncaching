
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
#include <deque>
#include <time.h>

#include "ns3/city.h"
#include "ns3/citycrc.h"
#include "ns3/core-module.h"
#include "ns3/CcnModule.h"
#include "ns3/experiment_globals.h"


namespace ns3 {

//  head next-prev tail
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

    //filename = filename + "-" + ID
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


        false_positive_cnt = 0;
        read_dram_cnt = 0;
        readcache_rmlru = 0;
        writecache_rmlru = 0;
        write_outoforder = 0;
        false_positive_cnt_w = 0;
        total_stored_packets = 0;
        sram_stored_packets = 0;
    }

    ~CacheModule(){
        //delete LRU;
    }
    
    //these used to be pure virtual but ns3 could not handle it
    virtual int32_t add_packet(const string& _filename, const string& ID, const char* _payload, const bool is_first_packet){return 0;}
    virtual uint32_t remove_last_packet(const string& _filename){return 0;}
    virtual int32_t get_cached_packet(const string& _filename, const string& ID){return 0;}
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
    

    uint64_t false_positive_cnt;
    uint64_t read_dram_cnt;
    uint64_t readcache_rmlru;
    uint64_t writecache_rmlru;
    uint64_t write_outoforder;
    uint64_t false_positive_cnt_w;
    uint64_t total_stored_packets;
    uint64_t sram_stored_packets;

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
   
    int32_t add_packet(const string& _filename, const string& _ID, const char* _payload, const bool is_first_packet);
    uint32_t remove_last_packet(const string& _filename);  
    int32_t remove_last_file();//new by argi
    int32_t get_stored_packets(const string& _filename);//new by argi
    int32_t get_cached_packet(const string& _filename, const string& _ID);
    uint32_t cache_packet(const string& _filename, const string& _ID, const char* _payload);
    string get_state();
    string get_packet_stats();
                
    uint64_t zero_pcks;   
    uint64_t added_pcks; 
};

class P_Cache: public CacheModule{
public:
    P_Cache(uint32_t _capacity, uint32_t _capacity_fast_table):CacheModule(_capacity, _capacity_fast_table){}
        
    int32_t get_cached_packet(const string& _filename, const string& _ID);
    uint32_t cache_packet(const string& _filename, const string& _ID, const char* _payload);
    int32_t add_packet(const string& _filename, const string& _ID, const char* _payload, const bool is_first_packet);
    uint32_t remove_last_packet(const string& _filename);
    
    string get_state();
    string get_packet_stats(){return "";}
     
};

typedef map<uint32_t, char *> Pkts;
class Slot_Object{
private:
    typedef map<string, uint8_t> Name2index;
    typedef vector<Pkts> Files;
    
    //map filename  to index in files variable,because file is vector
    Name2index name2index;
    Files files;
    uint32_t pkt_num = PKT_NUM;
    uint32_t file_num = FILE_NUM;
    uint32_t cur_index = 0;

public:
    //uint64_t write_for_evictions = 0;
    Slot_Object(){}

    
    /*
     *@func: insert a packet of filename
     *@param: make_pair(filename-ID, data_pointer)
     *@return: False,failed to insert. 
     *         True, succeeded to insert it or it has already been stored in DRAM
     *         if slot is full, instead the most front filename.
     *         int, stored_packet number at this time
     */
     pair<bool, int> insert_packet(const string& key, uint32_t _ID, char *payload);

    /*
     *@func: insert PKT_NUM or total(if it is less than PKT_NUM) packets of filename
     *@param: filename with begin-ID
     *@param: last_id, last_id subtracted  begin_id is less than PKT_NUM
     *@return: False,failed to insert; 
     *         True, succeeded to insert it or it has already been stored in DRAM
     *         if slot is full, instead the most front filename.
     *         int, stored_packet number at this time
     */
    pair<bool, int> insert_packets(string key, uint32_t last_id, Pkts payloads);

    /*
     *@param: filename without file ID(begin-id)
     *@return: bool,true or false
     *         Pkts
     */
    pair<bool, Pkts> find(const string & key);

    //File_Obj files[FILE_NUM];
     
};

class S_Cache:public CacheModule{
public:
    S_Cache(uint32_t _capacity, uint32_t _capacity_fast_table, map<string, uint32_t> *_file_map_p, double _fp):
                     CacheModule(_capacity,_capacity_fast_table){
        file_map_p = _file_map_p;
        stored_packets = 0;
        stored_files = 0;
        zero_pcks=0;
        readcache_pcks=0;
        writecache_pcks=0;
        LRU_W = new LRU_Table();

        if(_capacity == 0){
            slot_num = DRAM_SIZE*(1024*1024*1024/PKT_NUM/PKT_SIZE);
        }else{
            slot_num = _capacity/PKT_NUM;
        }
       
        if(_fp){
            index_bf_ptr = init_bf(_fp);
        }else{
            index_bf_ptr = init_bf(0.001);
        }
    }
    
    ~S_Cache(){
       delete index_bf_ptr;
    }

    // <filename, max chunk id>
//    map<string , uint32_t> index_table;
    bf::a2_bloom_filter  *index_bf_ptr;
    bf::a2_bloom_filter  *init_bf(double fp);

    map<string, uint32_t> *file_map_p;

    // DRAM table
    map <uint32_t, Slot_Object> data_table;

    //SRAM table for reading
    //filename-begin_id, packets
    typedef map<string, Pkts > Cachetable;
    Cachetable cache_table_r;

    //SRAM table for writing
    //filename-beigin_id, packets
    Cachetable cache_table_w;
    LRU_Table *LRU_W;

    // <filename, allocated memory (IN PACKETS!)
    map<string , uint32_t> mem_table;
    // <filename, stats metric> metric may be a successfull hit counter, and will be used for pacekt removals
    map<string , uint32_t> stats_table;

//    int32_t add_packet(const string& _filename, const string& _ID, const char* _payload, const bool is_first_packet);
    bool is_last(const string &_filename, const uint32_t ID);
    int32_t add_packet(const string& _filename, const uint32_t ID,const uint32_t block_id,  const char *_payload);
    int32_t transfer_packets(const string& filename, const uint32_t ID);
    uint32_t remove_last_packets_r(const string& _filename);
    int32_t remove_last_file_r();//new by argi
    uint32_t remove_last_packets_w(const string& _filename);
    int32_t remove_last_file_w();//new by argi
    int32_t get_stored_packets_r(const string& _filename);//new by argi
    int32_t get_stored_packets_w(const string& _filename);//new by argi
    int32_t get_cached_packet(const string& _filename, const string& _ID);
    uint32_t cache_packet(const string& _filename, const string& _ID, const char* _payload);
    void log_file_hit(const string& _filename, const string& _ID);
//    string get_state();
//    string get_packet_stats();

    uint64_t zero_pcks;
    uint64_t readcache_pcks;
    uint64_t writecache_pcks;
    // the number of slot, one slot has FILE_NUM files, one file has equal to or less than PKT_NUM packets
    uint64_t slot_num;
};

}
#endif
