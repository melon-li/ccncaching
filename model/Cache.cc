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
	//	std::cout<<"Added packet in cache "<<_filename<<" "<<ID<<std::cout ;

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
//	char* data;
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
		obj->next = obj;
		obj->prev = obj;
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
		/* always removing tail*/
		obj->next->prev = NULL;
		tail = obj->next;
		stored_files--;
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
	
}//ns3 namespace
