#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>      /* printf, fgets */
#include <stdlib.h>
#include <fstream>      // std::ifstream
#include <string.h>
#include <algorithm>    // std::sort


using namespace std;

int main(int argc, char** argv){
	char* log_filename = argv[1];
	unsigned limit_percent=100;	
	if (argc>2)
		limit_percent= atoi(argv[2]);
	std::ifstream myfile (log_filename);
	   string file,line;
	   
	   unsigned cum_hits=0;
	   string hit_items_str="";
	  
	   map<string, unsigned> occupancy;
	   map<string, unsigned>::iterator it;

	   char mode = 0; //0 is object
	if (myfile.is_open()){
		unsigned No=0;
		cout<<"Parsing logs\n";
		while ( getline (myfile,line) )	{
			int start_ln = line.find("hit_files");
			int start_log = line.find("/domain1/");
			
			if (start_ln==string::npos || start_log == string::npos){
				continue;
			}
			hit_items_str = line.substr(start_ln+10, start_log-(start_ln+10));
			printf("No. %d hit-files %s\n", No, hit_items_str.c_str());
			line = line.substr(line.find("/domain1/")) ;
			string item = line.substr(0, line.find(","));
			//printf("*Line %s\n", line.substr(0,20).c_str());
			//printf("*Item %s\n", item.substr(0,20).c_str());

			unsigned hits_int=0;
			string name, hits ;
			int pos;
			while (true){
				pos = item.find(":");
				if (pos==string::npos)
					break;
				name = item.substr(0,pos);
				//string id = item.substr(item.find("-")+1, item.find(":")+1);
				hits = item.substr(pos+1);
				hits_int = atoi(hits.c_str());
				cum_hits += hits_int;
				if (occupancy.find(name)!=occupancy.end())			
					occupancy[name]+= hits_int;
				else 
					occupancy[name]=hits_int;
				if (hits_int>100)
					printf("%s %d\n", name.c_str(),hits_int);
				line = line.substr(line.find(",")+1) ;
				item = line.substr(0, line.find(","));				
			}
			No++;
		}
		myfile.close();
		
		vector <std::pair<float, string> > sort_vec;

		float avg_hits_per_item = cum_hits/(float)occupancy.size();
		unsigned max_hits=0;
		vector<string> keys;
		vector<unsigned> objs;
		for (it=occupancy.begin();it!=occupancy.end();++it){
			if (it->second>max_hits){
				max_hits = it->second;
				printf("max hits %s %d\n", it->first.c_str(), max_hits);
			}
			sort_vec.push_back(std::make_pair(it->second, it->first));

			
		}
		
		printf("Cache logs: %d , cum_hits %d , items_hit: %d , avg_hits_per_item %f , max-hits %d\n", No, cum_hits, occupancy.size(), avg_hits_per_item, max_hits);

		//sort by value using std::sort
		std::sort(sort_vec.begin(), sort_vec.end());
		for (unsigned i=0 ;i<sort_vec.size();i++)
			printf("%s %f\n", sort_vec.at(i).second.c_str(),sort_vec.at(i).first);
	

			
	  }
	  else std::cout << "Unable to open file_map file\n"; 
	
	
			

	
	}
