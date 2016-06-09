#include "Cache.h"

namespace ns3 {
    
    NS_LOG_COMPONENT_DEFINE("Cache");


/**
 * Looks for cached packet in Object organised cache
 * returns 0 if packet is not found
 * returns >0  if packet is found cached
 * the actual data are never stored/retrieved
 */
uint32_t O_Cache::get_cached_packet(const string& _filename, const string& _ID){
    requests++;
    //click_chatter("Searching chunk.. %d", ID);
    unsigned ID = atoi(_ID.c_str());
    map<string, unsigned>::iterator it = index_table.find(_filename);
    // chunk is cached
    if (it!=index_table.end() && ID<=it->second){    
        LRU->update_object(LRU->objects[_filename]);//fere to arxeio san most recent
        string key = _filename+"-";
        key.append(_ID);
        map<string, unsigned>::iterator lit = log_file_hits.find(key);
        if (lit!=log_file_hits.end())
            lit->second++;
        else{
            log_file_hits[key]=1;
        }
        log_chunk_id_hits[std::min((unsigned) MAX_LOG_CHUNK_ID-1,ID)]++;
        hits++;
        //std::cout<<"Got hit for ID "<<ID<<" it->second:"<<(unsigned)it->second<<std::endl;
        uint16_t tmp = (it->second - ID + 1);
        reads_for_fetchings += tmp;
        return tmp * DRAM_ACCESS_TIME;
    }
    return 0;
}
    
/**
 * Called upon the receiption of a data packet.
 * Decides wether packet should be stored or not.
 * @payload is currently not used in the simulation
 */
uint32_t O_Cache::cache_packet(const string& _filename, const string& _ID, const char* _payload){
    responses++;
    unsigned lookup_time = 0;
    unsigned ID = atoi(_ID.c_str());
    map<string, unsigned>::iterator it = index_table.find(_filename);
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
            stored_packets-=removed_packets;
            lookup_time += removed_packets;
            reads_for_evictions+=removed_packets;
        }else if (stored_packets >= capacity){
            if (remove_last_packet(_filename)==0)
                return 0;
            stored_packets--;
            reads_for_evictions++;
        }
        uint16_t tmp = add_packet(_filename, _ID, _payload, true);
        lookup_time += tmp;
        reads_for_insertions+= tmp;
        stored_packets++;
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
        uint16_t tmp = add_packet(_filename, _ID, _payload, false);
        lookup_time += tmp;
        stored_packets++;
        reads_for_insertions+= tmp;
    //    std::cout<<"Added packet in cache "<<_filename<<" "<<ID<<std::cout ;

    }
    NS_LOG_INFO("O_Cache stored "<<_filename<<"/"<<_ID<<" capacity "<<index_table.size()<<" - "<<stored_packets);
    return lookup_time * DRAM_ACCESS_TIME;    
    }
    
/**
 * data packet is stored to memory
 * This operation is currently avoided in the simulation
 * Return accesses in DRAM 
 */
uint32_t O_Cache::add_packet(const string& _filename, const string& _ID, const  char* _payload, const bool is_first_packet){
    
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
            
    unsigned last_packet_id = index_table[last_filename];
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
    map<string, unsigned>::iterator it = index_table.find(_filename);
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
 * Looks for cached packet in Packet-organised cache
 * returns the amount of time spend in DRAM
 */
uint32_t P_Cache::get_cached_packet(const string& _filename, const string& _ID){
    requests++;
    string key = _filename+"-";
    key.append(_ID);
    map<string, char*>::iterator it = data_table.find(key);
    if (it!=data_table.end()){    
        LRU->update_object(LRU->objects[key]);
        map<string, unsigned>::iterator it = log_file_hits.find(key);
        if (it!=log_file_hits.end())
            it->second++;
        else
            log_file_hits[key]=1;    
        hits++;
        log_chunk_id_hits[std::min(MAX_LOG_CHUNK_ID-1,atoi(_ID.c_str()))]++;
        NS_LOG_DEBUG("Found key: "<<key<<" cached");
        reads_for_fetchings++;
        return 1 * DRAM_ACCESS_TIME;
        }
    return 0;

    }
    
// returns the lookup delay in *DRAM* (picosecond) 
uint32_t P_Cache::cache_packet(const string& _filename, const string& _ID, const char* _payload){
    string key = _filename+"-";
    key.append(_ID);
    unsigned access_time = 0;
    map<string, char*>::iterator it = data_table.find(key);
    /* chunk is already stored, do nothing */
    if (it!=data_table.end())
        return 0;
    /* remove the last chunk from the least recently used file*/
    if (stored_packets >= capacity){
        remove_last_packet(key); // do not add DRAM delay, overwritte wth one access
        stored_packets--;
        reads_for_evictions++;
    }
    access_time += add_packet(_filename, _ID, _payload, true);
    it = data_table.find(key);
    stored_packets++;
    //NS_LOG_DEBUG("Added packet "<<_filename<<"/"<<_ID<<" stored_pcks:"<<stored_packets<<"/"<<capacity);
    reads_for_insertions++;
    return access_time* DRAM_ACCESS_TIME;    
}
    
/**
 *  is_first_packet is not used in packet level caching
 * @payload is not used in the simulation
 * */    
uint32_t P_Cache::add_packet(const string& _filename, const string& _ID, const  char* _payload, const bool is_first_packet){
    string key = _filename+"-";
    key.append(_ID);
    //stats_table.set (_filename, 0);
    char* data = new char[PAYLOAD_SIZE];
    //memcpy(data, _payload->data(), PAYLOAD_SIZE);
    data_table.insert (std::pair<string,char*>(key, data));
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
        /* removing tail*/
        if(obj == tail){
            obj->next->prev = NULL;
            tail = obj->next;
            stored_files--;
        }else{
            obj->next->prev = obj->prev;
            obj->prev->next = obj->next;
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

pair<bool, int> Slot_Object::insert_packet(pair<string, char *> pkt){
    Name2index::iterator it;
    uint32_t pos = 0, _ID = 0;
    string _filename;
    int stored_packets = 0;

    pos = pkt.first.find("-");
    if(!pos) return std::make_pair(false, 0);
    _ID = atoi(pkt.first.substr(pos+1).c_str());
    _filename = pkt.first.substr(0, pos);
    it = name2index.find(_filename);

    if(it == name2index.end()){
        if(files.size() >= file_num){
            stored_packets -= files[cur_file].size();
            files[cur_file].clear();
            files[cur_file].insert(Pkts::value_type(_ID, pkt.second)); 
            stored_packets++;
            for(Name2index::iterator it_t=name2index.begin(); it_t!=name2index.end(); it_t++){
                if(it_t->second == cur_file) {
                    name2index.erase(it_t);
                    break;
                }
            }
            name2index[_filename] = cur_file;
            cur_file++;
            if(cur_file > file_num) cur_file = 0;
        }else{
            Pkts p;
            pos = files.size();
            p.insert(Pkts::value_type(_ID, pkt.second));
            files.push_back(p);
            stored_packets++;
            name2index.insert(Name2index::value_type(_filename, pos));
        }
    }else{
        pos = it->second;
        if(pos >= files.size()) return std::make_pair(false, 0);

        Pkts::iterator vt = files[pos].begin();
        if((_ID - vt->first) >= PKT_NUM) return std::make_pair(false, 0);

        if(files[pos].size() > PKT_NUM) return std::make_pair(false, 0);

        files[pos].insert(Pkts::value_type(_ID, pkt.second));
        stored_packets++;
    }
    return std::make_pair(true, stored_packets);
}

pair<bool, int> Slot_Object::insert_packets(string filename, uint8_t last_id, std::deque<char *> payloads){
    uint32_t pos = 0, _ID = 0, len=0;
    string _filename;
    int stored_packets = 0;

    pos = filename.find("-");
    if(!pos) return std::make_pair(false, 0);
    _ID = atoi(filename.substr(pos+1).c_str());
    _filename = filename.substr(0, pos);
    len = last_id - _ID + 1;
    if( len > PKT_NUM ) return std::make_pair(false, 0);
    if( len != payloads.size() )return std::make_pair(false, 0);

    for(uint8_t i =0; i< len; i++){
        _filename = _filename + "-";
        _filename.append(std::to_string(i));
        pair<bool, int> pr = insert_packet(std::make_pair(_filename, payloads[i]));
        stored_packets += pr.second;
        if(!pr.first)return std::make_pair(false, 0);
    }
 
    return std::make_pair(true, stored_packets);
}

pair<bool, Pkts> Slot_Object::find(string filename){
    Pkts p;
    Name2index::iterator it = name2index.find(filename);
    if(it == name2index.end()) return std::make_pair(false, p);
    return std::make_pair(true, files[it->second]);
}


int32_t S_Cache::remove_last_file_r(){
    string tail = LRU->tail->filename;
    int32_t removed_packets = get_stored_packets_r(tail);
    if (removed_packets == -1)
        return -1;
    cache_table_r.erase(tail);
    LRU->remove_object(LRU->tail);
    NS_LOG_INFO("S_Cache removed last file "<<tail);
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

//tranfer packets from dram to sram
uint32_t S_Cache::tranfer_packets(const string& filename){
    uint32_t lookup_time = 0;
    uint32_t addr = 0;
    addr = CityHash32(filename.c_str(), filename.size());
    addr = addr%slot_num;
    map <uint32_t, Slot_Object>::iterator it = data_table.find(addr);
    if(it == data_table.end()) return 0;
    pair<bool, Pkts> pr = it->second.find(filename);
    if(!pr.first) return 0;
     

    std::deque<char *> payloads;
    for(Pkts::iterator i=pr.second.begin(); i!=pr.second.end(); i++)
        payloads.push_back(i->second);    
   
    while(readcache_pcks + payloads.size() >= capacity_fast_table){
            int32_t removed_packets = remove_last_file_r();
            if (removed_packets == -1)
                return 0;
            readcache_pcks -= removed_packets;
    }
              
    lookup_time = (PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME*payloads.size() + \
                                      DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
   
    payloads.pop_front(); 
    cache_table_r.insert(Cachetable::value_type(filename, payloads));
    LRU_Object * _obj  = new LRU_Object(filename);
    LRU->add_object(_obj);	 
    readcache_pcks += payloads.size();  
    
    return lookup_time;
}

void S_Cache::log_file_hit(const string& _filename, const string& _ID){
    string key = _filename+"-";
    key.append(_ID);
    map<string, unsigned>::iterator lit = log_file_hits.find(key);
    if (lit!=log_file_hits.end())
        lit->second++;
    else{
        log_file_hits[key]=1;
    }
}

uint32_t S_Cache::get_cached_packet(const string& _filename, const string& _ID){
    uint32_t lookup_time = 0;
    unsigned ID = atoi(_ID.c_str()) - CACHING_START_INDEX;
    uint32_t block_id = ID/PKT_NUM;
    string filename(_filename);
    
    requests++;
    filename += "-";
    filename.append(std::to_string((block_id*PKT_NUM)));

    Cachetable::iterator cit = cache_table_r.find(filename);
    // if packets is cached in sram for reading,response them to requester,return 0
    if(cit != cache_table_r.end()){
        //uint32_t offset = ID - (block_id*PKT_NUM);
        //only support request cache by order at this moment
        //if(!offset){
        cit->second.pop_front();
        if(cit->second.size() == 0) {
            cache_table_r.erase(cit);
            LRU->remove_object(LRU->objects[filename]);
        }else{
            LRU->update_object(LRU->objects[filename]);
        }
        readcache_pcks--;
        //}
        log_file_hit(_filename, _ID);
        return 0;
    }

    //if not cached in "sram for reading", and check if stored in dram
    uint8_t iscache = index_bf.lookup(filename.c_str()); 
    if(!iscache) return 0;

    // stored in dram, tranfer them from dram to sram
    log_file_hit(_filename, _ID);
    lookup_time = tranfer_packets(filename); 
    return lookup_time;
}

int32_t S_Cache::remove_last_file_w(){
    string tail = LRU->tail->filename;
    int32_t removed_packets = get_stored_packets_w(tail);
    if (removed_packets == -1)
        return -1;
    cache_table_w.erase(tail);
    LRU->remove_object(LRU->tail);
    NS_LOG_INFO("S_Cache removed last file "<<tail);
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


int32_t S_Cache::add_packet(const string& _filename, const uint32_t ID,const uint32_t block_id,  const char *_payload){
    unsigned lookup_time = 0;
    bool islast = false;
    uint32_t addr = 0;
    char* data = new char[PAYLOAD_SIZE];
    Cachetable::iterator it = cache_table_w.find(_filename);
    if(it != cache_table_w.end()){
        it->second.push_back(data);
        writecache_pcks++;
        //up to  PKT_NUM or islast is true
        //if cache PKT_NUM packets or all packets(less than PKT_NUM) of a file, transfer these to DRAM
        if(it->second.size() >= PKT_NUM || islast){
           addr = CityHash32(_filename.c_str(), _filename.size());
           addr = addr%slot_num; 
           pair<bool, int> pr = data_table[addr].insert_packets(_filename, ID, it->second); 
           stored_packets += pr.second;
           cache_table_w.erase(it);
           lookup_time = (PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME*it->second.size() + \
                         DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
         }
    //is first packet
    }else{
        std::deque<char *> payloads;
        payloads.push_back(data);
        writecache_pcks++;
        NS_ASSERT_MSG(ID == block_id*PKT_NUM,
        "S_Cache::cache_packet:Internal error.Filename stored in SRAM cache for writing is wrong");            
        cache_table_w.insert(Cachetable::value_type(_filename, payloads));
        if(islast){
           addr = CityHash32(_filename.c_str(), _filename.size());
           addr = addr%slot_num; 
           pair<bool, int> pr = data_table[addr].insert_packets(_filename, ID, payloads); 
           cache_table_w.erase(it);
           stored_packets += pr.second;
           lookup_time = (PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME*1 + \
                         DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
        }else{
            LRU_Object * _obj  = new LRU_Object(_filename);
            LRU->add_object(_obj);		
        } 
    }  
}

uint32_t S_Cache::cache_packet(const string& _filename, const string& _ID, const char* _payload){
    string filename(_filename);
    unsigned lookup_time = 0;
    unsigned ID = atoi(_ID.c_str()) - CACHING_START_INDEX;
    unsigned block_id = ID/PKT_NUM;

    filename = filename + "-";
    filename.append(std::to_string(block_id*PKT_NUM)); 

    responses++;
    // if packet has existed in dram, return 0
    uint8_t iscache = index_bf.lookup(filename.c_str()); 
    if(iscache) return 0;

    //if sram is full, delete the least recent file
    if(writecache_pcks >= capacity_fast_table){
	    int32_t removed_packets = remove_last_file_w();
	    if (removed_packets == -1)
		return 0;			
	    writecache_pcks -= removed_packets;
	   // lookup_time += removed_packets;
	   // reads_for_evictions += removed_packets;
    }

    //cache them in "SRAM for writing(cache_table_w)"
    //if up to PKT_NUM or islast, then transfer them from sram to dram
    lookup_time = add_packet(filename, ID, block_id, _payload);
    NS_LOG_INFO("S_Cache stored "<<_filename<<"/"<<_ID<<" capacity ");
    return lookup_time;	
}
    
}//ns3 namespace
