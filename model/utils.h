/*
 * utils.h
 *
 *  Created on: Feb 11, 2014
 *      Author: tsilochr
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <vector>
#include <string>
#include <sstream>

using std::vector;
using std::string;
using std::stringstream;

namespace ns3{

//taken from stackoverflow

class Utils{
public:

    static
    vector<string> & split(const string &s, char delim, vector<string> &elems) {
        stringstream ss(s);
        string item;
        while (getline(ss, item, delim)) {
            elems.push_back(item);

        }
        return elems;
    }

    static
    vector<string> split(const string &s, char delim) {
        std::vector<std::string> elems;
        split(s, delim, elems);
        return elems;
    }

    static
    void replaceAll(std::string& str, const std::string& from, const std::string& to) {
        if(from.empty())
            return;
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    }
};


}
#endif /* UTILS_H_ */
