        /* Delete the oldest file, or in options, we can use LRU algorithm to delete file
           Note: it's worthy to discuss if we should delete the corresponding elements in bloom filter, 
        */
        NS_LOG_WARN("This slot is full(test)");
        NS_ASSERT_MSG(files[cur_index].size()!=0,
                     "File is empty");
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
        if(cur_index >= file_num) cur_index = 0;
        return std::make_pair(true, cnt);
    }

    //3.The file named key was not  cached!, and this slot is not full
    Pkts p;
    // get the pos after insert one element into files
    pos = files.size();
    p.insert(Pkts::value_type(_ID, payload));
    files.push_back(p);
    cnt++;
    name2index.insert(Name2index::value_type(key, pos));
    return std::make_pair(true, cnt);
}

