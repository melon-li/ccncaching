#include "Cache.h"

namespace ns3 {
    
    NS_LOG_COMPONENT_DEFINE("Cache");


/**
 * Looks for cached packet in Object organised cache
 * returns 0 if packet is not found
 * returns >0  if packet is found cached
 * the actual data are never stored/retrieved
 */
int32_t O_Cache::get_cached_packet(const string& _filename, const string& _ID){
    requests++;
    //click_chatter("Searching chunk.. %d", ID);
    uint32_t lookup_time = 0;
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
        lookup_time = DRAM_ACCESS_TIME + (PKT_SIZE/WIDTH -1)*DRAM_OLD_ACCESS_TIME + (tmp-1)*DRAM_ACCESS_TIME;
        return lookup_time;
       // return tmp * DRAM_ACCESS_TIME;
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

    }
    NS_LOG_INFO("O_Cache stored "<<_filename<<"/"<<_ID<<" capacity "<<index_table.size()<<" - "<<stored_packets);
    uint32_t tt = DRAM_ACCESS_TIME + (PKT_SIZE/WIDTH -1)*DRAM_OLD_ACCESS_TIME + (lookup_time-1)*DRAM_ACCESS_TIME;
    return tt;
  //  return lookup_time * DRAM_ACCESS_TIME;    
    }
    
/**
 * data packet is stored to memory
 * This operation is currently avoided in the simulation
 * Return accesses in DRAM 
 */
int32_t O_Cache::add_packet(const string& _filename, const string& _ID, const  char* _payload, const bool is_first_packet){
    
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
int32_t P_Cache::get_cached_packet(const string& _filename, const string& _ID){
    requests++;
    uint32_t lookup_time; 
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
        lookup_time = DRAM_ACCESS_TIME + (PKT_SIZE/WIDTH -1)*DRAM_OLD_ACCESS_TIME;
        return lookup_time;
        }
    return 0;

    }
    
// returns the lookup delay in *DRAM* (picosecond) 
uint32_t P_Cache::cache_packet(const string& _filename, const string& _ID, const char* _payload){
    string key = _filename+"-";
    key.append(_ID);
    uint32_t access_time = 0;
    uint32_t lookup_time = 0;
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
    lookup_time = DRAM_ACCESS_TIME + (PKT_SIZE/WIDTH -1)*DRAM_OLD_ACCESS_TIME + (access_time-1)*DRAM_ACCESS_TIME;
    return lookup_time;
}
    
/**
 *  is_first_packet is not used in packet level caching
 * @payload is not used in the simulation
 * */    
int32_t P_Cache::add_packet(const string& _filename, const string& _ID, const  char* _payload, const bool is_first_packet){
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

    //The file named key is not cached!
    if(it == name2index.end()){
        //this slot is full
        if(files.size() >= file_num){
            /* Delete the oldest file, or in options, we can use LRU algorithm to delete file
               Note: it's worthy to discuss if we should delete the corresponding elements in bloom filter, 
            */
            cnt -= files[cur_index].size();
            files[cur_index].clear();
            files[cur_index].insert(Pkts::value_type(_ID, payload)); 
            cnt++;
            //Find old corresponding element in name2index, and erase it
            for(Name2index::iterator it_t=name2index.begin(); it_t!=name2index.end(); it_t++){
                if(it_t->second == cur_index) {
                    name2index.erase(it_t);
                    break;
                }
            }
            //insert new element into name2index
            name2index.insert(Name2index::value_type(key, cur_index));

            // increment cur_index to can delete  the oldest file
            cur_index++;
            if(cur_index > file_num) cur_index = 0;
        }else{
            Pkts p;

            // get the pos after insert one element into files
            pos = files.size();
            p.insert(Pkts::value_type(_ID, payload));
            files.push_back(p);
            cnt++;
            name2index.insert(Name2index::value_type(key, pos));
        }
    }else{
        pos = it->second;
        //NS_ASSERT_MSG(pos < files.size, "Slot_Object::insert_packet, Internal error,pos can not be more than files.size");
        Pkts::iterator vt = files[pos].begin();
        NS_ASSERT_MSG((_ID - vt->first) < PKT_NUM, "Slot_Object::insert_packet, Internal error, out-of-order");
        NS_ASSERT_MSG(files[pos].size() <= PKT_NUM, "Slot_Object::insert_packet, Internal error, files can not be more than PKT_NUM");

        files[pos].insert(Pkts::value_type(_ID, payload));
        cnt++;
    }
    return std::make_pair(true, cnt);
}

pair<bool, int> Slot_Object::insert_packets(string key, uint8_t last_id, Pkts payloads){
    uint32_t pos = 0, _ID = 0, len=0;
    string _filename, filename;
    int cnt = 0;

    pos = key.find("-");
    if(!pos) return std::make_pair(false, 0);
    _ID = atoi(key.substr(pos+1).c_str());
    _filename = key.substr(0, pos);
    _filename = _filename + "-";

    len = last_id - _ID + 1;
    NS_ASSERT_MSG(len <= PKT_NUM, "Slot_Object::insert_packets, Internal error,len can not be more than  PKT_NUM");
    //if( len > PKT_NUM ) return std::make_pair(false, 0);
    
    //insert packets to dram
    for(Pkts::iterator it =payloads.begin(); it != payloads.end(); it++){
        //filename = _filename;
        //filename.append(std::to_string(it->first));
        pair<bool, int> pr = insert_packet(key, it->first, it->second);
        NS_ASSERT_MSG(pr.first, "Slot_Object::insert_packets, Internal error,insert_packet is failed");
        //if(!pr.first)return std::make_pair(false, 0);
        cnt += pr.second;
    }
 
    return std::make_pair(true, cnt);
}

pair<bool, Pkts> Slot_Object::find(const string& key){
    Pkts p;
    Name2index::iterator it = name2index.find(key);
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

void print_data_table(map<uint32_t, Slot_Object> & dt){
    for(map<uint32_t, Slot_Object>::iterator it= dt.begin(); it!=dt.end(); it++){
        NS_LOG_WARN("addr="<<(it->first));
    }
}
//Tranfer packets from dram to sram
int32_t S_Cache::transfer_packets(const string& key){
    uint32_t lookup_time = 0;
    uint32_t addr = 0;
    addr = CityHash64(key.c_str(), key.size());
    addr = addr%slot_num;
    map <uint32_t, Slot_Object>::iterator it = data_table.find(addr);
    //print_data_table(data_table);
    if(it == data_table.end()) return -1;
    pair<bool, Pkts> pr = it->second.find(key);
    if(!pr.first) return -1;
    //NS_LOG_WARN("Flag2");
     

    //find filename, trasfer map payloads to deque payloads
    //std::deque<char *> payloads;
    //for(Pkts::iterator i=pr.second.begin(); i!=pr.second.end(); i++)
    //    payloads.push_back(i->second);    
   
   //if readcache is full , delete LRU files
    while(readcache_pcks + pr.second.size() >= capacity_fast_table){
            int32_t removed_packets = remove_last_file_r();
            if (removed_packets == -1)
                return -1;
            readcache_pcks -= removed_packets;
            readcache_rmlru++;
    }
              
   
    // delete the packet requested at this time.
   // payloads.pop_front(); 
    pr.second.erase(pr.second.begin());

    cache_table_r.insert(Cachetable::value_type(key, pr.second));
    LRU_Object * _obj  = new LRU_Object(key);
    LRU->add_object(_obj);	 
    readcache_pcks += pr.second.size();  
    
    // cal lookup_time
    lookup_time = (PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME*(pr.second.size()) + \
                                      DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
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

int32_t S_Cache::get_cached_packet(const string& _filename, const string& _ID){
    uint32_t lookup_time = 0;
    unsigned ID = atoi(_ID.c_str()) - CACHING_START_INDEX;
    uint32_t block_id = ID/PKT_NUM;
    string key(_filename);
    
    requests++;
    key += "-";
    key.append(std::to_string((block_id*PKT_NUM)));

    Cachetable::iterator cit = cache_table_r.find(key);
    // if packets is cached in sram for reading,response them to requester,return 0
    if(cit != cache_table_r.end()){
        uint32_t offset = ID%PKT_NUM;
        Pkts::iterator pit = cit->second.find(offset);
        //if request one packet repeatedly, do not response
        if(pit ==  cit->second.end())return -1;
        cit->second.erase(pit);
        if(cit->second.size() == 0) {
            cache_table_r.erase(cit);
            LRU->remove_object(LRU->objects[key]);
        }else{
            LRU->update_object(LRU->objects[key]);
        }
        readcache_pcks--;
        //}
        log_file_hit(_filename, _ID);
        return 0;
    }

    //if not cached in "sram for reading", and check if stored in dram
    size_t iscache = index_bf_ptr->lookup(key.c_str()); 
    //NS_LOG_WARN("Bloom Filter:False positive ("<<key<<" iscache="<<iscache<<")");
    if(!iscache) return -1;

    read_dram_cnt++;
    // stored in dram, tranfer them from dram to sram
    log_file_hit(_filename, _ID);
    lookup_time = transfer_packets(key); 
    if(lookup_time == -1){
        false_positive_cnt++;
        //NS_LOG_WARN("Bloom Filter:False positive");
        return 0;
    }
    return lookup_time;
}

int32_t S_Cache::remove_last_file_w(){
    string tail = LRU_W->tail->filename;
    int32_t removed_packets = get_stored_packets_w(tail);
    if (removed_packets == -1)
        return -1;
    cache_table_w.erase(tail);
    LRU_W->remove_object(LRU_W->tail);
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


bool S_Cache::is_last(const string &_filename, const uint32_t ID){
   uint32_t filesize = ID + CACHING_START_INDEX; 
   string key = _filename.substr(_filename.rfind('/') + 1);
   map<string, uint32_t>::iterator it = (*file_map_p).find(key); 
   if(it == (*file_map_p).end()){
        NS_LOG_ERROR("Error. Do not find file("<<key<<")");
        return false;
   }
   if(filesize == it->second) return true;
   if(filesize > it->second) NS_LOG_ERROR("Error.filesize can not be more than "<<it->second);
   return false;
}

//cache packets and transfer them to dram when the number is more than PKT_NUM or is last one in file
int32_t S_Cache::add_packet(const string& key, const uint32_t ID, const uint32_t block_id, const char *_payload){
    unsigned write_time = 0;
    bool islast = false;
    uint32_t addr = 0;
    char* data = new char[PAYLOAD_SIZE];
    //memcpy(data, _payload->data(), PAYLOAD_SIZE);

    //if writecache is full, delete the least recent file
    if(writecache_pcks >= capacity_fast_table){
            NS_LOG_WARN("writecache_pcks >= "<<capacity_fast_table<<"----------------------------------");
	    int32_t removed_packets = remove_last_file_w();
	    if (removed_packets == -1){
                NS_LOG_ERROR("Internal error. Fail to remove packets in writecache!"); 
		return 0;	
            }		
	    writecache_pcks -= removed_packets;
            writecache_rmlru++;
	   // write_time += removed_packets;
	   // reads_for_evictions += removed_packets;
    }

    string filename = key.substr(0, key.find("-"));
    islast = is_last(filename, ID);

    Cachetable::iterator it = cache_table_w.find(key);
    NS_LOG_WARN("caching "<<key);
    if(it != cache_table_w.end()){
        //if the packet has exited or is out-of-order, ignore it and return 0
        if(ID != it->second.size()) return 0;

        it->second.insert(Pkts::value_type(ID, data));
        writecache_pcks++;

        //up to PKT_NUM or islast is true
        if(it->second.size() >= PKT_NUM || islast){
            addr = CityHash64(key.c_str(), key.size());
            addr = addr%slot_num; 
            map <uint32_t, Slot_Object>::iterator dtit = data_table.find(addr);
            pair<bool, int> pr;
            if(dtit == data_table.end()){
                NS_LOG_WARN("New data_table.len="<<data_table.size()<<" "<<key<<"->"<<slot_num<<":"<<addr<<" Flag1");
                Slot_Object so;
                pr = so.insert_packets(key, ID, it->second);
                data_table.insert(map<uint32_t, Slot_Object>::value_type(addr, so));
            }else{
                NS_LOG_WARN("old data_table.len="<<data_table.size()<<" "<<key<<"->"<<slot_num<<":"<<addr<<" Flag1");
                pr = dtit->second.insert_packets(key, ID, it->second);
            }
            NS_ASSERT_MSG(pr.first, "Error.Fail to insert_packets");
            index_bf_ptr->add(key.c_str());
            stored_packets += pr.second;
            writecache_pcks -= it->second.size();
            LRU_W->remove_object(LRU_W->objects[key]);
            cache_table_w.erase(it);
            write_time = (pr.second)*(PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME + \
                         DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
         }else{
            LRU_W->update_object(LRU_W->objects[key]);
         }
    //is first packet
    }else{
        Pkts payloads;
        payloads.insert(Pkts::value_type(ID, data));
        NS_ASSERT_MSG(ID == block_id*PKT_NUM,"Internal error.packet is not the first.\n\
                                              Filename stored in SRAM cache for writing is wrong");            
        if(islast){
            addr = CityHash64(key.c_str(), key.size());
            addr = addr%slot_num; 
            map <uint32_t, Slot_Object>::iterator dtit = data_table.find(addr);
            pair<bool, int> pr;
            if(dtit == data_table.end()){
                NS_LOG_WARN("New data_table.len="<<data_table.size()<<" "<<key<<"->"<<slot_num<<":"<<addr<<" Flag1");
                Slot_Object so;
                pr = so.insert_packets(key, ID, payloads);
                data_table.insert(map<uint32_t, Slot_Object>::value_type(addr, so));
            }else{
                NS_LOG_WARN("old data_table.len="<<data_table.size()<<" "<<key<<"->"<<slot_num<<":"<<addr<<" Flag1");
                pr = dtit->second.insert_packets(key, ID, payloads);
            }
            NS_ASSERT_MSG(pr.first, "Error.Fail to insert_packets");
            index_bf_ptr->add(key.c_str());
            stored_packets += pr.second;
            write_time = (PKT_SIZE/WIDTH)*DRAM_OLD_ACCESS_TIME*1 + \
                         DRAM_ACCESS_TIME - DRAM_OLD_ACCESS_TIME;
         }else{
            cache_table_w.insert(Cachetable::value_type(key, payloads));
            writecache_pcks++;
            LRU_Object * _obj  = new LRU_Object(key);
            LRU_W->add_object(_obj);		
         } 
    }  
    return write_time;
}

//cache a packet
uint32_t S_Cache::cache_packet(const string& _filename, const string& _ID, const char* _payload){
    string key(_filename);
    unsigned write_time = 0;
    unsigned ID = atoi(_ID.c_str()) - CACHING_START_INDEX;
    unsigned block_id = ID/PKT_NUM;

    key = key + "-";
    key.append(std::to_string(block_id*PKT_NUM)); 

    responses++;
    // if packet has existed in dram, ignore and return 0
    size_t iscache = index_bf_ptr->lookup(key.c_str()); 
    if(iscache) return 0;

    /*cache them in "SRAM for writing(cache_table_w)"
    if up to PKT_NUM or islast, then transfer them from sram to dram*/
    write_time = add_packet(key, ID, block_id, _payload);
    NS_LOG_INFO("S_Cache stored "<<_filename<<"/"<<_ID);
    return write_time;	
}
    

bf::a2_bloom_filter *S_Cache::init_bf(double fp){
    NS_ASSERT_MSG(capacity, "capacity can not be zero");
    NS_ASSERT_MSG(fp, "fp can not be zero");
    size_t ka; //The number of hash function for fp
    size_t cells; //bits, the number of cells to use
    ka = std::floor(-std::log(1 - std::sqrt(1 - fp)) / std::log(2));
    cells = 2*ka*(capacity/PKT_NUM)*std::log(2);
    NS_LOG_INFO("ka = "<<ka<<" cells = "<<cells);
    return new bf::a2_bloom_filter{ka, cells, capacity/PKT_NUM, 1, 199};
}

}//ns3 namespace
