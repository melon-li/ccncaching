#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <stdio.h>      /* printf, fgets */
#include <stdlib.h>

using namespace std;

int main (int argv , char** args){
	  string line;
	  ifstream myfile (args[1]);
	  map <unsigned, unsigned> nodes;
	  unsigned node_id, counter;
	  if (myfile.is_open()){
		while ( getline (myfile,line) )	{
		 // cout << line << '\n';
		  node_id = atoi (line.substr(0, line.find(" ")).c_str());
		  //cout << node_id << '\n';
		  counter = 0;
		  while (line.find("<")!=string::npos){
			counter++;
			line = line.substr(line.find("<")+1);
		  }
		  nodes[node_id] = counter;
		}
		myfile.close();
	  }
	  else cout << "Unable to open file"; 
	
      cout<<"V: "<<nodes.size()<<endl;	 
      unsigned E=0;
      for (map<unsigned, unsigned>::iterator it=nodes.begin();it!=nodes.end();++it )
	  	E+=it->second;
	  	
	  E = E/2; //its bidirectional
      cout<<"E: "<<E<<endl;	 
      cout<<"E/V: "<<((double)E/(double)nodes.size())<<endl;	 

  return 0;
	
}
