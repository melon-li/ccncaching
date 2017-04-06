#include "Cache.h"

namespace ns3 {
    
    NS_LOG_COMPONENT_DEFINE("Cache");


/**
 * Looks for cached packet in Object organised cache
 * returns 0 if packet is not found
 * returns >0  if packet is found cached
 * the actual data are never stored/retrieved
 */
pair<int64_t, int64_t> O_Cache::get_cached_packet(const string& _filename, const string& _ID){
    requests++;
    increase_file_requests(_filename, _ID);
    int64_t lookup_time = 0;
    uint32_t ID = atoi(_ID.c_str());
    map<string, uint32_t>::iterator it = index_table.find(_filename);
    // Chunk is cached
    if (it!=index_table.end() && ID<=it->second){    
        LRU->update_object(LRU->objects[_filename]);
        string key = _filename+"-";
        key.append(_ID);
        map<string, uint32_t>::iterator lit = log_file_hits.find(key);
        if (lit!=log_file_hits.end())
            lit->second++;
        else{
            log_file_hits[key]=1;
        }
        log_chunk_id_hits[std::min((uint32_t) MAX_LOG_CHUNK_ID-1,ID)]++;
        //hits++;
        uint32_t req = get_file_requests(_filename, _ID);
        clear_file_requests(_filename, _ID);
        hits += req;
        //NS_LOG_DEBUG("Got hit for ID "<<ID<<" it->second:"<<(uint32_t)it->second);
        uint16_t tmp = (it->second - ID + 1);
        //std::cout<<"tmp="<<tmp<<std::endl;
        //reads_for_fetchings += tmp;
        reads_for_fetchings++;
        lookup_time = (PKT_SIZE/WIDTH -1)*DRAM_OLD_ACCESS_TIME + tmp*DRAM_ACCESS_TIME;
        return std::make_pair(0, lookup_time);
       // return tmp * DRAM_ACCESS_TIME;
    }
    miss++;
    return std::make_pair(-1, 0);
}
    
/**
 * Called upon the receiption of a data packet.
 * Decides wether packet should be stored or not.
 * @payload is currently not used in the simulation
 */
uint64_t O_Cache::cache_packet(const string& _filename, const string& _ID, const char* _payload){
    responses++;
    uint32_t req = get_file_requests(_filename, _ID);
    req = req == 1?req-1:req;
    hits += req;
    clear_file_requests(_filename, _ID);

    uint32_t lookup_time = 0;
    uint32_t ID = atoi(_ID.c_str());
    map<string, uint32_t>::iterator it = index_table.find(_filename);
    /* always store the first packet */
    if (ID == CACHING_START_INDEX){    
        /* chunk is already stored, do nothing */
        if (it!=index_table.end())
            return 0;
        zero_pcks++;
        /* remove the least recently used file*/
        if (index_table.size() >= capacity_fast_table){
            int32_t removed_packets=remove_last_file();
            if (removed_packets==-1)
                return 0;            
            stored_packets -= removed_packets;
            lookup_time += removed_packets;
            reads_for_evictions+=removed_packets;
        }else if (stored_packets >= capacity){
            if (remove_last_packet(_filename)==0)
                return 0;
            lookup_time++;
            stored_packets--;
            reads_for_evictions++;
        }
        int64_t tmp = add_packet(_filename, _ID, _payload, true);
        lookup_time += tmp;
        reads_for_insertions += tmp;
        stored_packets++;
        write_for_storings++;
        //std::cout<<"Added packet in cache\n";

    }    
    /* if previous packet is stored */        
    else if (it!=index_table.end() && ID == (it->second+1)){
        added_pcks++;
        if (stored_packets >= capacity){
            if (remove_last_packet(_filename)==0)
                return 0;
            lookup_time ++;
            stored_packets--;
            reads_for_evictions++;
        }
        int64_t tmp = add_packet(_filename, _ID, _payload, false);
        lookup_time += tmp;
        stored_packets++;
        reads_for_insertions+= tmp;

    }
    NS_LOG_INFO("O_Cache stored "<<_filename<<"/"<<_ID<<" capacity "<<index_table.size()<<" - "<<stored_packets);
    uint32_t tt = (PKT_SIZE/WIDTH -1)*DRAM_OLD_ACCESS_TIME + lookup_time*DRAM_ACCESS_TIME;
    return tt;
  //  return lookup_time * DRAM_ACCESS_TIME;    
    }
    
/**
 * data packet is stored to memory
 * This operation is currently avoided in the simulation
 * Return accesses in DRAM 
 */
int64_t O_Cache::add_packet(const string& _filename, const string& _ID, const  char* _payload, const bool is_first_packet){
    
    uint32_t ID = atoi(_ID.c_str());
    /* if its packet 0 then have to add it to caching tables */
//    char* data;
    if (is_first_packet){
        index_table.insert(std::pair<string, uint32_t>(_filename, CACHING_START_INDEX));
        LRU_Object * _obj  = new LRU_Object(_filename);
        LRU->add_object(_obj);        
    }
    /* have to store payload and update counters */
    else {
        index_table[_filename] = ID;
    }
    return 1;
 }

/***
 * Removes the last chunk of the least recently used file
 * Reduces chunk counter and resizes allocated memory space when needed
 * Returns accesses in DRAM
 * */
uint32_t O_Cache::remove_last_packet(const string& new_packet_filename){
    string last_filename = LRU->tail->filename;
    
    if (last_filename.compare(new_packet_filename)==0) 
            return 0;
            
    uint32_t last_packet_id = index_table[last_filename];
    /* if its the LAST chunk remove all table items*/
    if (last_packet_id==CACHING_START_INDEX){
        index_table.erase(last_filename);
        LRU->remove_object(LRU->tail);    
        return 1;
        }
    /* if its NOT the LAST chunk */
    last_packet_id--;
    index_table[last_filename]=last_packet_id;
    
    NS_LOG_INFO("O_Cache removed last packet "<<last_filename<<" capacity "<<index_table.size()<<" - "<<stored_packets);
    return 1;
}

/***
 * Removes the least recently used filename
 **/
int32_t O_Cache::remove_last_file(){
    string tail = LRU->tail->filename;
    int32_t removed_packets=get_stored_packets(tail);
    if (removed_packets==-1)
        return -1;
    index_table.erase(tail);
    LRU->remove_object(LRU->tail);
    NS_LOG_INFO("O_Cache removed last file "<<tail<<" capacity "<<index_table.size()<<" - "<<stored_packets-removed_packets);
    return removed_packets;
}

/***
 * Returns the number of the stored packets in the file _filename or 0 if filename not found
 **/
int32_t O_Cache::get_stored_packets(const string& _filename){
    map<string, uint32_t>::iterator it = index_table.find(_filename);
    if (it!=index_table.end()){
        return it->second;
    }
    else return -1;
}

string O_Cache::get_state(){
    std::stringstream ss, ss1;
    string s="";
    s+= "O_cache| ";
    ss1<<stored_packets;
    ss << index_table.size();
    s+=" indexSize:"+ ss.str() +" stored_packets "+ ss1.str()+ "\t";
    for (map<string , uint32_t>::iterator it=index_table.begin();it!=index_table.end();++it){
        ss.str(std::string());
        ss<<it->second;
        s+=it->first+"-"+ss.str()+",";
    }
    s+="\n\n";
    
    return s;
}
    
string CacheModule::get_file_hits(){
    std::stringstream ss;
    string s="";
    ss << log_file_hits.size();
    s+= "_cache | hit_files:"+ ss.str() +"\t";
    for (map<string , uint32_t>::iterator it=log_file_hits.begin();it!=log_file_hits.end();++it){
        ss.str(std::string());
        ss<<it->second;
        s+=it->first+":"+ss.str()+",";
    }
    s+="\n\n";
    log_file_hits.clear();

    return s;
    }
    
string CacheModule::get_chunk_id_hits(){
    std::stringstream ss, ss1;
    string s="";
    s+= "_cache | chunk-id hits\t";
    for (uint32_t l=0;l<MAX_LOG_CHUNK_ID;l++){
        if (log_chunk_id_hits[l]>0){
            ss<<log_chunk_id_hits[l];
            ss1<<l;
            s+=ss1.str()+":"+ss.str()+",";
            ss1.str(std::string());
            ss.str(std::string());
        }
    }
    s+="\n\n";
    return s;
}

uint32_t CacheModule::get_file_requests(const string &_filename, const string& _ID){
    string key = _filename;
    key = key + "-";
    key.append(_ID);
    map<string, uint32_t>::iterator lit = log_file_requests.find(key);
    if (lit!=log_file_requests.end()){
        if(lit->second == 1){// it is not first time to access data  has been cached
            return 1;
        }else if(lit->second > 1){//it is first time to access data has been cached
            return lit->second - 1;
        }
    }
    return 1;
}

void CacheModule::increase_file_requests(const string &_filename, const string& _ID){
    string key = _filename;
    key = key + "-";
    key.append(_ID);
    map<string, uint32_t>::iterator lit = log_file_requests.find(key);
    if (lit!=log_file_requests.end())
        lit->second++;
    else{
        log_file_requests[key] = 1;
    }
}

void CacheModule::clear_file_requests(const string &_filename, const string& _ID){
    string key = _filename;
    key = key + "-";
    key.append(_ID);
    map<string, uint32_t>::iterator lit = log_file_requests.find(key);
    if (lit != log_file_requests.end()){
        log_file_requests.erase(lit);
    }
}

string O_Cache::get_packet_stats(){
    std::stringstream ss;
    string s="";
    ss << requests ;
    s+= "_cache | requests: "+ ss.str() +" ";
    ss.str("");
    ss << responses ;
    s+= "responses: "+ ss.str() +" ";
    ss.str("");
    ss << zero_pcks ;
    s+= "zero_pcks: "+ ss.str() +" ";
    ss.str("");
    ss << added_pcks ;
    s+= "added_pcks: "+ ss.str() +" ";
    ss.str("");
    
    s+="\n\n\n";
    log_file_hits.clear();

    return s;
}
    
string P_Cache::get_state(){
    std::stringstream ss;
    string s="";
    ss << data_table.size();
    s+= "P_cache | indexSize:"+ ss.str() +"\t";
    for (map<string , char*>::iterator it=data_table.begin();it!=data_table.end();++it){
        s+=it->first+",";
    }
    s+="\n\n\n";
    return s;
    }

/**
 * Looks for cached packet in Packet-organised cache.
 * returns the amount of time spend in DRAM.
 */
pair<int64_t, int64_t> P_Cache::get_cached_packet(const string& _filename, 
                                                  const string& _ID){
    requests++;
    increase_file_requests(_filename, _ID);
    int64_t lookup_time = 0; 
    string key = _filename + "-";
    key.append(_ID);
    map<string, char*>::iterator it = data_table.find(key);

    if (it != data_table.end()){    
        LRU->update_object(LRU->objects[key]);
        map<string, uint32_t>::iterator it = log_file_hits.find(key);
        if (it!=log_file_hits.end())
            it->second++;
        else
            log_file_hits[key]=1;    
        //hits++;
        uint32_t req = get_file_requests(_filename, _ID);
        clear_file_requests(_filename, _ID);
        hits += req;

        log_chunk_id_hits[std::min(MAX_LOG_CHUNK_ID-1,atoi(_ID.c_str()))]++;
        NS_LOG_DEBUG("Found key: "<<key<<" cached");
        reads_for_fetchings++;
        //lookup_time = DRAM_ACCESS_TIME + 
        //              (PKT_SIZE/WIDTH -1)*DRAM_OLD_ACCESS_TIME;
        //lookup_time = LRU_ACCESS_TIME; // for-test
        lookup_time = PKT_SIZE*8*1000/LRU_RATE; //unit is ps
        return std::make_pair(0, lookup_time);
    }
    miss++;
    return std::make_pair(-1, 0);
}
    
// It returns the lookup delay in *DRAM* (picosecond). 
uint64_t P_Cache::cache_packet(const string& _filename, const string& _ID, const char* _payload){
    responses++;
    uint32_t req = get_file_requests(_filename, _ID);
    req = req == 1?req-1:req;
    hits += req;
    clear_file_requests(_filename, _ID);

    string key = _filename+"-";
    key.append(_ID);
    int64_t access_time = 0;
    uint64_t lookup_time = 0;
    map<string, char*>::iterator it = data_table.find(key);

    
    /* chunk is already stored, do nothing */
    if (it != data_table.end()) return 0;
    /* remove the last chunk from the least recently used file*/
    if (stored_packets >= capacity){
        // Not add DRAM delay, overwritte wth one access.
        remove_last_packet(key); 
        stored_packets--;
        reads_for_evictions++;
    }
    access_time += add_packet(_filename, _ID, _payload, true);
    it = data_table.find(key);
    stored_packets++;
    //NS_LOG_DEBUG("Added packet "<<_filename<<"/"<<_ID
    //             <<" stored_pcks:"<<stored_packets
    //             <<"/"<<capacity);
    reads_for_insertions++;
    //lookup_time = DRAM_ACCESS_TIME + 
    //              (PKT_SIZE/WIDTH -1)*DRAM_OLD_ACCESS_TIME + 
    //              (access_time-1)*DRAM_ACCESS_TIME;
    //lookup_time = LRU_ACCESS_TIME;
    lookup_time = PKT_SIZE*1000*8/LRU_RATE; // Unit is ps.
    return lookup_time;
}
    
/**
 *  is_first_packet is not used in packet level caching
 * @payload is not used in the simulation
 * */    
int64_t P_Cache::add_packet(const string& _filename, const string& _ID, const  char* _payload, const bool is_first_packet){
    string key = _filename+"-";
    key.append(_ID);
    //stats_table.set (_filename, 0);
    //char* data = new char[PAYLOAD_SIZE];
      char *data = NULL;
    //memcpy(data, _payload->data(), PAYLOAD_SIZE);
    data_table.insert (std::pair<string,char*>(key, data));
    write_for_storings++;
    NS_LOG_DEBUG("Added key "<< key<<" data 1st byte "<< *(data));
    LRU_Object * _obj  = new LRU_Object(key);
    LRU->add_object(_obj);        
    return 1;
}

/***
 * _filename is actually the key filename+ID
 * */
uint32_t P_Cache::remove_last_packet(const string& _filename){
    string key = LRU->tail->filename;
    //stats_table.erase(key);
    delete [] data_table[key];
    data_table.erase(key);
    LRU->remove_object(LRU->tail);    
    NS_LOG_DEBUG("Removed packet "<<key);

    return 1;
}
    
void LRU_Table::add_object(LRU_Object* obj){
    /* first item in cache */
    if (stored_files == 0){
        obj->next = NULL;
        obj->prev = NULL;
        head = obj;
        tail = obj;
        stored_files=1;
        }
    else{
        stored_files++;
        update_object(obj, true);    
        }
        
        objects.insert (std::pair<string,LRU_Object*>(obj->filename, obj));
    }
    
void LRU_Table::remove_object(const LRU_Object* obj){
    /* first cache is empty  (maybe this check is redundant) */
    if (stored_files == 0){
        return;
        }
    else if (stored_files == 1){
        /* head and tail is the same obj*/
        head = NULL;
        tail = NULL;
        stored_files = 0;
        }
    else{
        /* if obj is  tail*/
        if(obj == tail){
            obj->next->prev = NULL;
            tail = obj->next;
            stored_files--;
        }else{
            // remove head
            if(obj->next == NULL){
                head = obj->prev;
                obj->prev->next = NULL;
            }else{
                obj->next->prev = obj->prev;
                NS_ASSERT_MSG(obj->prev, "LRU_Table::remove_object.Error. The poiter is NULL");
                obj->prev->next = obj->next;
            }
            stored_files--;
        }
    }
    objects.erase(obj->filename);
    delete obj;    
}
    
/**
 *  sets object as head
 */
void LRU_Table::update_object(LRU_Object* obj, bool new_object){
    
    if (new_object){
        head->next = obj;
        obj->prev = head;
        head = obj;
        }
    else if (stored_files > 1 && head!=obj){
        
        if (tail == obj){
            tail = obj->next;
            obj->next->prev = NULL;
            }
        else{
            obj->next->prev = obj->prev;
            obj->prev->next = obj->next;
            }
        obj->next = NULL;
        obj->prev = head;
        head->next = obj;
        head = obj;
        }
    }

pair<bool, int> Slot_Object::insert_packet(const string& key, uint32_t _ID, char *payload){
    Name2index::iterator it;
    uint32_t pos = 0;
    string _filename;
    int cnt = 0;
    it = name2index.find(key);
    //1.The file named key was  cached partly!, 
    //cache its packet without checkout order.
    if(it != name2index.end()){
        pos = it->second;
        Pkts::iterator vt = files[pos].begin();
        NS_ASSERT_MSG((_ID - vt->first) < PKT_NUM,
           "Slot_Object::insert_packet, Internal error, out-of-order"
            << _ID << " " << vt->first);
        NS_ASSERT_MSG(files[pos].size() <= PKT_NUM,
           "Slot_Object::insert_packet, Internal error,"
            << "files can not be more than PKT_NUM");
        files[pos].insert(Pkts::value_type(_ID, payload));
        cnt++;
        return std::make_pair(true, cnt);
     }

     //2.The file named key was not cached!, and this slot is full.
    if(files.size() >= file_num){
        /* Delete the oldest file, or in options, 
         * we can use LRU algorithm to delete file
           Note: it's worthy to discuss if we should 
           delete the corresponding elements in bloom filter, 
        */
        //NS_LOG_WARN("This slot is full(test)");
        NS_ASSERT_MSG(files[cur_index].size() != 0,
                     "File is empty");
        cnt -= files[cur_index].size();
        files[cur_index].clear();
        files[cur_index].insert(Pkts::value_type(_ID, payload)); 
        cnt++;
        //Find old corresponding element in name2index, and erase it
        for (Name2index::iterator it_t=name2index.begin(); 
             it_t!=name2index.end(); it_t++){
            if(it_t->second == cur_index) {
                name2index.erase(it_t);
                break;
            }
        }
        //insert new element into name2index
        name2index.insert(Name2index::value_type(key, cur_index));

        // increment cur_index to can delete  the oldest file
        cur_index++;
        if(cur_index >= file_num) cur_index = 0;
        return std::make_pair(true, cnt);
    }

    // 3.The file named key was not  cached!, and this slot is not full
    Pkts p;
    // Get the pos after insert one element into files
    pos = files.size();
    p.insert(Pkts::value_type(_ID, payload));
    files.push_back(p);
    cnt++;
    name2index.insert(Name2index::value_type(key, pos));
    return std::make_pair(true, cnt);
}

pair<bool, int> Slot_Object::insert_packets(const string& key, 
                                            uint32_t last_id,
                                            Pkts pkts){
    int cnt = 0;
    checkout_file(key, last_id); 
    //insert packets to dram
    for (Pkts::iterator it = pkts.begin(); it != pkts.end(); it++){
        pair<bool, int> pr = insert_packet(key, it->first, it->second);
        NS_ASSERT_MSG(pr.first, "Internal error,insert_packet is failed");
        cnt += pr.second;
    }
    return std::make_pair(true, cnt);
}

inline void Slot_Object::checkout_file(const string &key, uint32_t last_id){
    uint32_t pos = 0, ID = 0, len = 0;
    pos = key.find("-");
    NS_ASSERT_MSG(pos,"Key format is error!");
    ID = atoi(key.substr(pos+1).c_str());
    len = last_id - ID + 1;
    NS_ASSERT_MSG(len <= PKT_NUM, "Internal error,"
                 << last_id << "-" << ID 
                 << ",len can not be more than  PKT_NUM");
}

pair<bool, Pkts> Slot_Object::find(const string& key){
    Pkts p;
    Name2index::iterator it = name2index.find(key);
    if(it == name2index.end()) return std::make_pair(false, p);
    return std::make_pair(true, files[it->second]);
}


int32_t S_Cache::remove_last_file_r(){
    string key = LRU->tail->filename;
    int32_t removed_packets = get_stored_packets_r(key);
    if (removed_packets == -1)
        return -1;
    cache_table_r.erase(key);
    LRU->remove_object(LRU->tail);
    NS_LOG_INFO("S_Cache removed last file "<<key);
    return removed_packets;
}

int32_t S_Cache::get_stored_packets_r(const string& _filename){
    Cachetable::iterator it = cache_table_r.find(_filename);
    if (it != cache_table_r.end()){
        return it->second.size();
    }else{
        return -1;
    }
}

void print_data_table(map<uint32_t, Slot_Object> & dt){
    for(map<uint32_t, Slot_Object>::iterator it= dt.begin(); it!=dt.end(); it++){
        NS_LOG_WARN("addr="<<(it->first));
    }
}


void S_Cache::log_file_hit(const string& _filename, const string& _ID){
    string key = _filename+"-";
    key.append(_ID);
    map<string, uint32_t>::iterator lit = log_file_hits.find(key);
    if (lit!=log_file_hits.end())
        lit->second++;
    else{
        log_file_hits[key]=1;
    }
}

//if return value >0:found,  <0: forward, 
//delete  packet after read to make room for residual data block from DRAM
int32_t S_Cache::get_readcached_packet2(const string& key, uint32_t ID){
    Cachetable::iterator cit = cache_table_r.find(key);
    if(cit != cache_table_r.end()){
        Pkts::iterator pit = cit->second.begin();
        //drop first packet,it is for compatibility with S_Cache::get_readcached_packet
        if(key.substr(key.rfind("-")+1) == std::to_string(ID-1) && pit->first == (ID-1)){
            cit->second.erase(pit);
            readcache_pcks--;
            pit = cit->second.begin();
        }
        if(ID != pit->first){
            NS_LOG_WARN("WARNING: out of order, key="<<key<<" ID="<<ID<<" Front="<<pit->first);
            return -1;
        }
        //NS_LOG_DEBUG("getcache:"<<key<<" "<<pit->first);
        cit->second.erase(pit);
        readcache_pcks--;
        /*
        //pit = cit->second.find(ID);
        //if request one packet repeatedly, do not response
        if(pit ==  cit->second.end()){
            std::cout<<"order: "<<key<<" "<<ID<<" "<<std::endl;
            for(Pkts::iterator it=cit->second.begin(); it!=cit->second.end();it++)
                std::cout<<it->first<<" ";
            std::cout<<std::endl;
   
             return 1;
             return -1;
        }*/

        if(cit->second.size() == 0) {
            cache_table_r.erase(cit);
            LRU->remove_object(LRU->objects[key]);
        }else{
            LRU->update_object(LRU->objects[key]);
        }
         
        uint32_t req = get_file_requests(key.substr(0, key.rfind("-")), 
                                              std::to_string(ID+1));
        clear_file_requests(key.substr(0, key.rfind("-")), 
                                              std::to_string(ID+1));
        hits += req;
        return 0;
    }
    return -1;
}

//if return value >=0:found,  <0: forward
int32_t S_Cache::get_readcached_packet(const string& key, const uint32_t ID){
    Cachetable::iterator cit = cache_table_r.find(key);
    if(cit == cache_table_r.end()) return -1;
    LRU->update_object(LRU->objects[key]);
    //log_file_hit(_filename, _ID);
    uint32_t req = get_file_requests(key.substr(0, key.rfind("-")), 
                                              std::to_string(ID+1));
    clear_file_requests(key.substr(0, key.rfind("-")), 
                                              std::to_string(ID+1));
    hits += req;
    return 0;
}

//if return value >0:found,  <0: forward
int32_t S_Cache::get_writecached_packet(const string& key, const uint32_t ID){
    Cachetable::iterator cit = cache_table_w.find(key);
    if(cit == cache_table_w.end()) return -1;
    Pkts::iterator pit = cit->second.find(ID);
    if(pit ==  cit->second.end()) return -1;
    //log_file_hit(_filename, _ID);
    uint32_t req = get_file_requests(key.substr(0, key.rfind("-")), 
                                              std::to_string(ID+1));
    clear_file_requests(key.substr(0, key.rfind("-")), 
                                              std::to_string(ID+1));
    hits += req;
    LRU_W->update_object(LRU_W->objects[key]);
    return 0;
}

//if readcache is full , delete LRU files
inline void S_Cache::checkout_readcache(const Pkts& pkts){
    if(!enable_opt){
        while(readcache_pcks + pkts.size() >= capacity_fast_table){
                int32_t removed_packets = remove_last_file_r();
                if (removed_packets == -1)
                    NS_ASSERT_MSG(false, "Failed to remove packets");
                readcache_pcks -= removed_packets;
                readcache_rmlru++;
        }
        return;
    }

    uint64_t opt_fast_table = uint64_t(capacity_fast_table*OPT_RATIO);
    while(readcache_pcks + pkts.size() >= opt_fast_table){
            int32_t removed_packets = remove_last_file_r();
            if (removed_packets == -1)
                NS_ASSERT_MSG(false, "Failed to remove packets");
            readcache_pcks -= removed_packets;
            readcache_rmlru++;
    }
}

int32_t S_Cache::get_avg_readtime(const string& key, const uint32_t ID){
    int32_t lookup_time = 0;
    uint64_t addr = 0;
    addr = CityHash64(key.c_str(), key.size());
    addr = addr%slot_num;
    map <uint32_t, Slot_Object>::iterator it = data_table.find(addr);
    //print_data_table(data_table);
    if(it == data_table.end()) return -1;
    pair<bool, Pkts> pr = it->second.find(key);
    if(!pr.first) return -1;
     
    // cal lookup_time
    lookup_time = (PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME*(pr.second.size()) + \
                                      DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
    return lookup_time/pr.second.size();
}

int32_t S_Cache::get_dram_packet(const string& key, const uint32_t ID){
    int64_t lookup_time = 0;
    uint64_t addr = 0;
    int32_t pcks_num = 0;
    addr = CityHash64(key.c_str(), key.size());
    addr = addr%slot_num;
    map <uint32_t, Slot_Object>::iterator it = data_table.find(addr);
    //print_data_table(data_table);
    if(it == data_table.end()) return -1;
    pair<bool, Pkts> pr = it->second.find(key);
    if(!pr.first) return -1;
     
    checkout_readcache(pr.second);

    pair<Cachetable::iterator, bool> result = 
             cache_table_r.insert(Cachetable::value_type(key, pr.second));
    // failed to insert, key may exist in cache_table_r
    if (result.second == false){
        if(cache_table_r.find(key) != cache_table_r.end()){
            pcks_num -= cache_table_r[key].size();
            LRU->remove_object(LRU->objects[key]);
            cache_table_r[key] = pr.second;
        }
    }

    LRU_Object * _obj  = new LRU_Object(key);
    LRU->add_object(_obj);     
    pcks_num += pr.second.size();  
    readcache_pcks += pcks_num;  
    slow_memory_hit +=  pcks_num;
    // cal lookup_time
    lookup_time = (PKT_SIZE/WIDTH)*pcks_num*DRAM_OLD_ACCESS_TIME + \
                              DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;

    //log_file_hit(_filename, _ID);
    uint32_t req = get_file_requests(
        key.substr(0, key.rfind("-")), 
        std::to_string(ID+1));
    clear_file_requests(
        key.substr(0, key.rfind("-")), 
        std::to_string(ID+1));
    hits += req;
    reads_for_fetchings++;
    return lookup_time;
}

pair<int64_t, int64_t> S_Cache::get_cached_packet(
                         const string& _filename,
                         const string& _ID){
    int64_t lookup_time = 0;
    uint32_t ID = atoi(_ID.c_str()) - CACHING_START_INDEX;
    requests++;
    increase_file_requests(_filename, _ID);

    string key(_filename);
    key += "-";
    key.append(std::to_string(((ID/PKT_NUM)*PKT_NUM)));
    //int32_t lt = get_avg_readtime(key, ID); 
    if(enable_opt){
        lookup_time = get_readcached_packet2(key, ID);
    }else{
        lookup_time = get_readcached_packet(key, ID);
    }
    if(lookup_time >=0){
        fast_memory_hit++;
        //return std::make_pair(0, LRU_ACCESS_TIME);
        return std::make_pair(0, 0);
    }

    lookup_time = get_writecached_packet(key, ID);
    if(lookup_time >=0){
        fast_memory_hit++;
        //return std::make_pair(0, LRU_ACCESS_TIME);
        return std::make_pair(0, 0);
    }

    //if not cached in sram, and checkout if stored in dram
    size_t iscache = index_bf_ptr->lookup(key.c_str()); 
    if(iscache == 0){
        miss++;
        return std::make_pair(-1, 0);
    }
    // for-test
    if (data_test.find(key) == data_test.end()){
        false_positive_cnt_test++;
    }
    
    read_dram_cnt++;
    lookup_time += DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME + 
                      DRAM_OLD_ACCESS_TIME*FILE_NUM*LRU_ENTRY_SIZE/WIDTH;
    int64_t lt = get_dram_packet(key, ID);  
    if(lt == -1){ 
        false_positive_cnt++;
        miss++;
        return std::make_pair(-1, lookup_time);
    }

    return std::make_pair(0, lookup_time + lt); 
}

int32_t S_Cache::remove_last_file_w(){
    string key = LRU_W->tail->filename;
    int32_t removed_packets = get_stored_packets_w(key);
    if (removed_packets == -1)
        return -1;
    cache_table_w.erase(key);
    LRU_W->remove_object(LRU_W->tail);
    NS_LOG_INFO("S_Cache removed last file "<<key);
    return removed_packets;
}

/***
 * Returns the number of the stored packets in the file _filename or 0 if filename not found
 **/
int32_t S_Cache::get_stored_packets_w(const string& _filename){
    Cachetable::iterator it = cache_table_w.find(_filename);
    if (it != cache_table_w.end()){
        return it->second.size();
    }else{
        return -1;
    }
}


pair<bool,uint32_t> S_Cache::is_last(const string &key, const uint32_t ID){
   uint32_t filesize = ID + CACHING_START_INDEX; 
   string filename = key.substr(0, key.find("-"));
   filename = filename.substr(filename.rfind('/') + 1);
   map<string, uint32_t>::iterator it = (*file_map_p).find(filename); 
   if(it == (*file_map_p).end()){
        NS_ASSERT_MSG(false, "Error. Do not find file("<<filename<<")");
   }
   if(filesize == it->second) return std::make_pair(true, it->second);
   if(filesize > it->second) NS_ASSERT_MSG(false, "Error.filesize can not be more than "<<it->second);
   return std::make_pair(false, it->second);
}

//divide into each cache due to a bug in ns3 
int32_t S_Cache::get_avg_writetime(const uint32_t ID, const uint32_t total_length){
    int32_t wt = 0;
    if(ID < (total_length/PKT_NUM)*PKT_NUM){
        wt = (PKT_NUM)*(PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME + \
                      DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
        return wt/PKT_NUM;
    }else{
        uint32_t s = total_length - (ID/PKT_NUM)*PKT_NUM;
        wt = (s)*(PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME + \
                      DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
        return wt/s;
            
    }
}

uint32_t S_Cache::store_packets(const string& key, 
                                const uint32_t last_id, 
                                const Pkts & pkts){
    uint64_t write_time = 0;
    uint64_t addr = 0;
    addr = CityHash64(key.c_str(), key.size());
    addr = addr%slot_num; 
    map <uint32_t, Slot_Object>::iterator dtit = data_table.find(addr);
    pair<bool, int> pr;
    if(dtit != data_table.end()) {
        pr = dtit->second.insert_packets(key, last_id, pkts);
    }else {
        Slot_Object so;
        pr = so.insert_packets(key, last_id, pkts);
        data_table.insert(map<uint32_t, Slot_Object>::value_type(addr, so));
    }
    NS_ASSERT_MSG(pr.first, "Error.Fail to insert_packets");
    index_bf_ptr->add(key.c_str());
    data_test.insert(key);  //for-test

    stored_packets += pr.second;
    total_stored_packets += pkts.size();
    
    write_time = (pkts.size())*(PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME + \
                 DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
    write_for_storings++;
    return write_time;
}

inline void S_Cache::checkout_writecache(){
    if(writecache_pcks >= capacity_fast_table){
        int32_t removed_packets = remove_last_file_w();
        if (removed_packets == -1){
            NS_ASSERT_MSG(false, "Internal error. Fail to remove packets in writecache!"); 
        }        
        writecache_pcks -= removed_packets;
        writecache_rmlru++;
    }
}


// Cache packets and transfer them to dram,
// when the number is more than PKT_NUM or is last one in file.
int64_t S_Cache::add_packet(const string& key, 
                            const uint32_t ID, 
                            const uint32_t chunk_id, 
                            const char *_payload){
    pair<bool,uint32_t> islast = is_last(key, ID);
    char* data = NULL;

    //If writecache is full, delete the LRU file.
    checkout_writecache();
    Cachetable::iterator it = cache_table_w.find(key);
    if(it != cache_table_w.end()){
        // If the packet has exited or is out-of-order,
        // ignore it and return 0.
        if((ID-chunk_id*PKT_NUM) != it->second.size()){
            write_outoforder++;
            return 0;
        }

        it->second.insert(Pkts::value_type(ID, data));
        writecache_pcks++;
        sram_stored_packets++;
        //Up to PKT_NUM or islast is true
        if(it->second.size() >= PKT_NUM || islast.first){
            uint32_t wt = store_packets(key, ID, it->second);
            LRU_W->remove_object(LRU_W->objects[key]);
            writecache_pcks -= it->second.size();
            cache_table_w.erase(it);
            return wt;
         }else{
            LRU_W->update_object(LRU_W->objects[key]);
         }
    }else{ //is first packet
        if(ID != chunk_id*PKT_NUM){
            NS_LOG_ERROR("Packet is not the first key="<<key
                <<" ID="<<ID<<" chunk_id*PKT_NUM="<<(chunk_id*PKT_NUM));
            write_outoforder++;
            return 0;
        }
        Pkts pkts;
        pkts.insert(Pkts::value_type(ID, data));
        if(islast.first){
             sram_stored_packets++;
             return store_packets(key, ID, pkts);
         }else{
            cache_table_w.insert(Cachetable::value_type(key, pkts));
            writecache_pcks++;
            sram_stored_packets++;
            LRU_Object * _obj  = new LRU_Object(key);
            LRU_W->add_object(_obj);        
         } 
    }  
    return 0;
}

bool S_Cache::is_reallycached(const string &key){
    uint64_t addr = CityHash64(key.c_str(), key.size());
    addr = addr%slot_num;
    map <uint32_t, Slot_Object>::iterator it = data_table.find(addr);
    if(it != data_table.end()){
        pair<bool, Pkts> pr = it->second.find(key);
        if(pr.first) return true;
    }
    return false;
}

//cache a packet
uint64_t S_Cache::cache_packet(const string& _filename,
                               const string& _ID,
                               const char* _payload){
    uint64_t write_time = 0;
    uint32_t ID = uint32_t(atoi(_ID.c_str()) - CACHING_START_INDEX);
    uint32_t chunk_id = ID/PKT_NUM;

    responses++;
    uint32_t req = get_file_requests(_filename, _ID);
    req = req == 1 ? req-1: req;
    hits += req;
    clear_file_requests(_filename, _ID);

    string key(_filename);
    key = key + "-";
    key.append(std::to_string(chunk_id*PKT_NUM)); 
    // if packet has existed in dram, ignore and
    // return time taked to look up tables.
    size_t iscache = index_bf_ptr->lookup(key.c_str()); 
    if(iscache){
        // for-test
        if (data_test.find(key) == data_test.end()) {
            false_positive_cnt_w_test++;
        }

        write_time += DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME +
                      (FILE_NUM*LRU_ENTRY_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME;
        if(is_reallycached(key)) return write_time;
        false_positive_cnt_w++;
    }

    //If not exist in dram, store it.
    write_time += add_packet(key, ID, chunk_id, _payload); 
    NS_LOG_INFO("S_Cache stored "<<_filename<<"/"<<_ID);
    return write_time;    
}

bf::a2_bloom_filter *S_Cache::init_bf(double fp){
    NS_ASSERT_MSG(capacity, "capacity can not be zero");
    NS_ASSERT_MSG(fp, "fp can not be zero");
    size_t ka; //The number of hash function for fp
    size_t cells; //bits, the number of cells to use 

    ka = std::floor(-std::log(1 - std::sqrt(1 - fp)) / std::log(2));
    cells = 2*ka*(capacity/PKT_NUM)/std::log(2);

    NS_LOG_UNCOND("Init A^2  BF,ka = " << ka 
                   << " fp = " << fp
                   << " cells = " << cells 
                   << " size = " << cells/1000/1000 << " Mb "
                   << " one bucket capacity = " << (capacity/PKT_NUM));

    return new bf::a2_bloom_filter{ka, cells, capacity/PKT_NUM, 1, 199};
}

/**
 * Looks for cached packet in DRAM-SSD cache
 * returns the amount of time spend in DRAM-SSD cache
 */
pair<int64_t, int64_t> D_Cache::get_cached_packet(const string& _filename, const string& _ID){
    //lookup_time, timeunit:ps
    int64_t lt = 0; 

    //For the statistics of requests and hits
    requests++;
    increase_file_requests(_filename, _ID);

    uint32_t ID = atoi(_ID.c_str()) - CACHING_START_INDEX;
    string chunk_name = _filename + "-";
    chunk_name.append(std::to_string(((ID/PKT_NUM)*PKT_NUM)));

    //DRAM_table is cached in SRAM
    Cachetable::iterator it = DRAM_table.find(chunk_name);
    if (it != DRAM_table.end()){
        Pkts::iterator pt = it->second.find(ID);
        if( pt == it->second.end()){
            miss++;
            return std::make_pair(-1, 0);
        }
        L1_LRU->update_object(L1_LRU->objects[chunk_name]);
        fast_memory_hit++;
    }

    // If packet is cached in DRAM or it stored in SSD, 
    // calculate the time to transfer packet from DRAM to ethernet.
    lt += (PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME + \
               DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;

    if (it == DRAM_table.end()){
        //SSD_table is cached in DRAM
        Cachetable::iterator it = SSD_table.find(chunk_name);
        
        //In fact, the lookup time can be much longer the following value.
        lt += LRU_ENTRY_SIZE*DRAM_OLD_ACCESS_TIME/WIDTH +
              DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME; 
        if(it == SSD_table.end()){
            miss++;
            return std::make_pair(-1, lt);
        }
        L2_LRU->update_object(L2_LRU->objects[chunk_name]);
        DRAM_table.insert(std::make_pair(it->first, it->second));
        LRU_Object * _obj  = new LRU_Object(chunk_name);
        L1_LRU->add_object(_obj);        
        //lt += (it->second.size()*PKT_SIZE*8*1000)/SSD_DATA_RATE; 
        lt += (it->second.size()*PKT_SIZE*8*1000)/SSD_READ_RATE; 
        slow_memory_hit += it->second.size();
        reads_for_fetchings++;
    }

    string key = _filename + "-";
    key.append(_ID);
    map<string, uint32_t>::iterator rit = log_file_hits.find(key);
    if (rit != log_file_hits.end())
        rit->second++;
    else
        log_file_hits[key] = 1;    
    //hits++;
    uint32_t req = get_file_requests(_filename, _ID);
    clear_file_requests(_filename, _ID);
    hits += req;

    log_chunk_id_hits[std::min(MAX_LOG_CHUNK_ID-1, atoi(_ID.c_str()))]++;
    NS_LOG_DEBUG("Found key: "<<key<<" cached");

    return std::make_pair(0, lt);
}

// Returns the lookup delay in SSD-DRAM caching system (picosecond)
uint64_t D_Cache::cache_packet(const string& _filename, 
                               const string& _ID, 
                               const char* _payload){
    int64_t wt = 0;

    //For statistic of request and hits
    responses++;
    uint32_t req = get_file_requests(_filename, _ID);
    req = req == 1?req-1:req;
    hits += req;
    clear_file_requests(_filename, _ID);

    // Prepare
    uint32_t ID = uint32_t(atoi(_ID.c_str()) - CACHING_START_INDEX);
    uint32_t chunk_id = ID/PKT_NUM;
    string chunk_name(_filename);
    chunk_name = chunk_name + "-";
    chunk_name.append(std::to_string(chunk_id*PKT_NUM)); 

    // If chunk has existed in ssd, then ignore and return 0
    Cachetable::iterator it = SSD_table.find(chunk_name); 
    if(it != SSD_table.end()) return wt;

    // If not exist in ssd,store it
    wt = add_packet(chunk_name, ID, chunk_id, _payload);
    NS_LOG_INFO("D_Cache stored "<<_filename<<"/"<<_ID);
    return wt;  
}

/*
 *Cache packets in DRAM, and then transfer them to SSD,
 *when the number is more than PKT_NUM or is last one in file.
 *
 *Parameters:
 *    chunk_name: string, _filename-chunk-id*PKT_NUM.
 *    ID: uint32_t, packet-id. 
 *    chunk_id: chunk-id.
 *    _payload: char*.
 *Return:
 *    int64_t: the time taken by DRAM-SSD caching system.
 */
int64_t D_Cache::add_packet(const string& chunk_name, 
                            const uint32_t ID,
                            const uint32_t chunk_id,
                            const char* _payload){
    char* data = 0;
    uint64_t wt = 0;
    pair<bool, uint32_t> islast = is_last(chunk_name, ID);

    // If dram cache is full, 
    // then delete file recently used in L1_LRU.
    checkout_dramcache();
    Cachetable::iterator it = DRAM_table.find(chunk_name);
    if(it != DRAM_table.end()){
        // If the packet has exited or is out-of-order, 
        // ignore it and return 0.
        if((ID - chunk_id*PKT_NUM) != it->second.size()){
            dramcache_outoforder++;
            return 0;
        }

        it->second.insert(Pkts::value_type(ID, data));
        L1_LRU->update_object(L1_LRU->objects[chunk_name]);
        // Assume that one packet is stored in a single row of DRAM.
        wt += (PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME + \
                 DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
        dramcache_pcks++;
        // If chunk size is up to PKT_NUM or islast return true.
        if(it->second.size() >= PKT_NUM || islast.first)
            wt += store_packets(chunk_name, it->second);
    // If it is the first packet.
    }else{ 
        if(ID != chunk_id*PKT_NUM){
            NS_LOG_ERROR("Packet is not the first chunk_name="<<chunk_name
                         <<" ID="<<ID
                         <<" chunk_id*PKT_NUM="<<(chunk_id*PKT_NUM));
            dramcache_outoforder++;
            return 0;
        }
        Pkts pkts;
        pkts.insert(Pkts::value_type(ID, data));
        if(islast.first){
           wt +=  store_packets(chunk_name, pkts);
        }else{
           DRAM_table.insert(Cachetable::value_type(chunk_name, pkts));
           dramcache_pcks++;
           LRU_Object * _obj  = new LRU_Object(chunk_name);
           L1_LRU->add_object(_obj);        
        } 
        // Assume that one packet is stored in a single row of DRAM.
        // Whatever it is the last packet or not, calculate the time to transfering it from DRAM to port.
        wt += (PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME + \
                 DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
    }  
    return wt;
}

uint32_t D_Cache::store_packets(const string& chunk_name,
                                const Pkts & pkts){
    uint64_t wt = 0;

    // If ssd cache is full, 
    // then delete file recently used in L1_LRU.
    checkout_ssd();
    if ( SSD_table.find(chunk_name) != SSD_table.end()){
        L2_LRU->update_object(L2_LRU->objects[chunk_name]);
        return wt;
    }else{
        LRU_Object* _obj  = new LRU_Object(chunk_name);
        L2_LRU->add_object(_obj);        
        SSD_table.insert(Cachetable::value_type(chunk_name, pkts));
        stored_packets += pkts.size();
        total_stored_packets += pkts.size();
        //wt += (pkts.size()*PKT_SIZE*8*1000)/SSD_DATA_RATE;
        wt += (pkts.size()*PKT_SIZE*8*1000)/SSD_WRITE_RATE;
    }
    write_for_storings++;
    return wt;
}

pair<bool,uint32_t> D_Cache::is_last(const string &chunk_name, 
                                     const uint32_t ID){
   uint32_t filesize = ID + CACHING_START_INDEX; 
   string filename = chunk_name.substr(0, chunk_name.find("-"));
   filename = filename.substr(filename.rfind('/') + 1);
   map<string, uint32_t>::iterator it = (*file_map_p).find(filename); 
   if(it == (*file_map_p).end()){
        NS_ASSERT_MSG(false, "Error. Do not find file("
                      <<filename<<")");
   }
   if(filesize == it->second) return std::make_pair(true, it->second);
   if(filesize > it->second) 
       NS_ASSERT_MSG(false, "Error.filesize can not be more than "
                     <<it->second);
   return std::make_pair(false, it->second);
}

inline void D_Cache::checkout_dramcache(){
    if(dramcache_pcks >= capacity_fast_table){
        int32_t removed_packets = remove_last_chunk_dram();
        if (removed_packets == -1){
            NS_ASSERT_MSG(false, 
                "Internal error. Fail to remove packets in writecache!"); 
        }        
        dramcache_pcks -= removed_packets;
        dramcache_rmlru++;
    }
}

int32_t D_Cache::remove_last_chunk_dram(){
    string chunk_name = L1_LRU->tail->filename;
    int32_t removed_packets = get_cached_packets_dram(chunk_name);
    if (removed_packets == -1)
        return -1;
    DRAM_table.erase(chunk_name);
    L1_LRU->remove_object(L1_LRU->tail);
    NS_LOG_INFO("D_Cache removed last file "<<chunk_name);
    return removed_packets;
}

/**
 * Returns the number of the stored packets in the chunk_name 
 * or 0 if chunk not found.
 **/
int32_t D_Cache::get_cached_packets_dram(const string& chunk_name){
    Cachetable::iterator it = DRAM_table.find(chunk_name);
    if (it != DRAM_table.end()){
        return it->second.size();
    }else{
        return -1;
    }
}

inline void D_Cache::checkout_ssd(){
    if(stored_packets >= capacity){
        int32_t removed_packets = remove_last_chunk_ssd();
        if (removed_packets == -1){
            NS_ASSERT_MSG(false, 
                "Internal error. Fail to remove packets in ssd!"); 
        }        
        stored_packets -= removed_packets;
        ssd_rmlru++;
    }
}

int32_t D_Cache::remove_last_chunk_ssd(){
    string chunk_name = L2_LRU->tail->filename;
    int32_t removed_packets = get_cached_packets_ssd(chunk_name);
    if (removed_packets == -1)
        return -1;
    SSD_table.erase(chunk_name);
    L2_LRU->remove_object(L2_LRU->tail);
    NS_LOG_INFO("D_Cache removed last file stored in SSD "<<chunk_name);
    return removed_packets;
}

/**
 * Returns the number of the stored packets in the chunk_name 
 * or 0 if chunk not found.
 **/
int32_t D_Cache::get_cached_packets_ssd(const string& chunk_name){
    Cachetable::iterator it = SSD_table.find(chunk_name);
    if (it != SSD_table.end()){
        return it->second.size();
    }else{
        return -1;
    }
}

}//ns3 namespace
