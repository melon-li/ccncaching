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
	   unsigned cached_items;
	   unsigned cum_cached_items=0;
	   unsigned packets=0;
	   unsigned cum_packets=0;
	   
	  
	   map<string, unsigned> occupancy;
	   map<string, unsigned>::iterator it;
	   map< unsigned, map<string, unsigned> > maps;
	   map< unsigned, map<string, unsigned> >::iterator it2 ;
	   
	   char mode = 0; //0 is object
	if (myfile.is_open()){
		unsigned No=0;
		cout<<"Parsing logs\n";
		while ( getline (myfile,line) )	{
			
			if (line.find("P_cache")!=string::npos)
				mode=1;
			 if (line.find("Size:")==string::npos)
				continue;
			 cached_items=atoi(line.substr(line.find("Size:")+5, line.find("\t")).c_str());
			 
			 //cout<<"No."<<No<<" Mode: "<<(unsigned) mode<<" Cached_items: "<<cached_items<<endl;
			 cum_cached_items+=cached_items;
			 if (cached_items==0) continue;
			 line = line.substr(line.find("\t")+1, line.length()) ;
			 string item = line.substr(0, line.find(","));

			 if (mode==1){
				 while (true){
					 string name = item.substr(0,item.find("-"));
					 string id = item.substr(item.find("-")+1); // id contains the number of cached items

					 if (occupancy.find(name)!=occupancy.end())			
						occupancy[name]++;
					 else 
						occupancy[name]=0; //MAYBE make this 1;
					 line = line.substr(line.find(",")+1, line.length()) ;
					 item = line.substr(0, line.find(","));
					 if (item.length()<4 || item.compare("")==0)
						break;
				 }
			 
			 }else if (mode==0){
				
				 while (true){
					string name = item.substr(0,item.find("-"));
					string id = item.substr(item.find("-")+1);

					occupancy[name] = atoi(id.c_str());
					//	cout<<"added file: "<<name<<", "<<id<<endl; 
					 line = line.substr(line.find(",")+1, line.length()) ;
					 item = line.substr(0, line.find(","));
					 if (item.length()<4 || item.compare("")==0)
						break;
				 }
			} else continue;
			maps[No]=occupancy;
			occupancy.clear();
			 No++;
		}
		myfile.close();

		printf("Cache logs: %ld (%ld), avg size %ld - cum %d \n", maps.size(), No, cum_cached_items/maps.size(), cum_cached_items);
		
		
		//for every state_log
		float avg_pck_per_file=0;
		float overall_avg_pck_per_file=0;
		unsigned max=0; 
		for (it2=maps.begin();it2!=maps.end();++it2){
			for (it=it2->second.begin();it!=it2->second.end();++it){
				avg_pck_per_file+=it->second;
				if (max<it->second)
					max=it->second;
			}
			//cout<<"avg pck per file "<< avg_pck_per_file<<endl;
			overall_avg_pck_per_file+=avg_pck_per_file / (float)it2->second.size();
			avg_pck_per_file=0;
			}
		printf("average packets per file: %f max_num_of_packets: %d\n",(float)overall_avg_pck_per_file/maps.size(), max);

		
		//parse occurencies
		map< unsigned, map<string, unsigned> >::iterator it3;
		map<string, float> all_files;
		map<string, float>::iterator it4;
		for (it2=maps.begin();it2!=maps.end();++it2){
			// for each cache log
			for (it=it2->second.begin();it!=it2->second.end();++it){
				//for each log item (file)
				string file = it->first;
				it4=all_files.find(file);
				if (it4!=all_files.end()){
					it4->second++;
					//cout<<"Found again file" <<file<<endl;
				}
				else
					all_files[file]=1;
			}
		}
		
		double avg_occurencies=0;
		unsigned max_occurencies=0;
		for (map<string, float>::iterator itt=all_files.begin();itt!=all_files.end();++itt){
			avg_occurencies+=itt->second;
			if (max_occurencies<itt->second){
				max_occurencies = itt->second;
				cout<<"max occurencies: "<<itt->first<<"  "<<itt->second <<endl;
			}
			
		}
		
		unsigned frags = 25;
		bool marked=false;
		unsigned* cdf = new unsigned [frags];
		for (unsigned j=0;j<10;j++)
				cdf[j]=0;
		for (map<string, float>::iterator itt=all_files.begin();itt!=all_files.end();++itt){
			marked=false;
			for (unsigned j=9;j>=1;j--){
				if (itt->second>max_occurencies*j/10){
					cdf[j]++;
					marked=true;
				//	break;
				}
			}
			if (!marked)
				cdf[0]++;
					
		}
		printf("Number of files %ld avg occurencies %f max occurencies %d \n",all_files.size(),avg_occurencies/all_files.size(), max_occurencies);	
		printf("CDF of caching frequency of files \nPercentage | #Files\n");
		for (unsigned j=0;j<10;j++)
			printf("%d %d\n",j*10, cdf[j]);
	
	
		//normalize occurencies
		for (map<string, float>::iterator itt=all_files.begin();itt!=all_files.end();++itt){
			itt->second = (itt->second/(float)maps.size())*100 ;	
			//printf ("%f %d \n",	itt->second, (float)max_occurencies	);
		}
		vector <std::pair<float, string> > sort_vec;
		for (unsigned j=0;j<frags;j++)
				cdf[j]=0;
		for (map<string, float>::iterator itt=all_files.begin();itt!=all_files.end();++itt){
			marked=false;
			sort_vec.push_back(std::make_pair(itt->second, itt->first));
			for (unsigned j=frags-1;j>=1;j--){
				if (itt->second>j*100/frags){
					cdf[j]++;
					if (j>=limit_percent)
						cout<<itt->first<<": "<<itt->second<<endl;
					marked=true;
					//break;
					}
			}
			if (!marked)
				cdf[0]++;
					
		}
		printf("Number of files %ld avg occurencies %f max occurencies %d \n",all_files.size(),avg_occurencies/all_files.size(), max_occurencies);	
		printf("CDF of caching frequency of files \nPercentage | #Files\n");
		for (unsigned j=0;j<frags;j++)
			printf("%d\n", cdf[j]);
			
		 
		////sort by value using std::sort
		std::sort(sort_vec.begin(), sort_vec.end());
		for (unsigned i=sort_vec.size()-350 ;i<sort_vec.size();i++)
			printf("%s %f\n", sort_vec.at(i).second.c_str(),sort_vec.at(i).first);
	
			
	  }
	  else std::cout << "Unable to open file_map file\n"; 
	

	
	}
