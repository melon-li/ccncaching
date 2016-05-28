#include "Graph.h"

using std::vector;

namespace ns3
{


    void Graph::Dijkstra(uint32_t source)
    {
        this->source=source;
        remember.resize(graph.size()+1);//we leave it as is ,filled with 0
        apostasi.resize(graph.size()+1);
        progonos.resize(graph.size()+1);
        std::cout<<"Graph: Graph size + 1: "<<graph.size()+1<<std::endl;
        for(uint32_t i=0;i<graph.size()+1;i++)
            apostasi.at(i)=100000;
        apostasi.at(source)=0;
        //construct adjacency matrix
        //1 ennooume oti syndeontai ,2 oti den syndeontai
        vector< vector <uint32_t> > matrix;//ti thesi miden de ti xrisimopoioume
        matrix.resize(graph.size()+1);

        for(uint32_t i=0;i<graph.size()+1;i++)
            matrix[i].resize(graph.size()+1);

        for(uint32_t i=0;i<graph.size()+1;i++)
            for(uint32_t j=0;j<graph.size()+1;j++)
                if( i!=j )//if i==j then it's 0 (resize())
                    matrix.at(i).at(j)=100000;

        for(uint32_t i=0;i<graph.size()+1;i++)
            progonos.at(i)=-1;

        map< uint32_t , set < uint32_t > >::iterator it;
        std::set<uint32_t>::iterator it2;
        for(it = graph.begin(); it != graph.end(); it++){
            std::set<uint32_t> set1=it->second;
            for(it2=set1.begin();it2!=set1.end();it2++)
                matrix[it->first][*it2]=1;//ola ta bari einai 1
        }
        //print adjacency matrix
        for(uint32_t i=0;i<graph.size()+1;i++)
        {
            for(uint32_t j=0;j<graph.size()+1;j++)
            {
                if(i!=0 && j!=0) 
                    std::cout<<matrix[i][j];

                if(matrix[i][j]<10)
                    std::cout<<"     ";
                else if(matrix[i][j]<100)
                    std::cout<<"    ";
                else if(matrix[i][j]<1000)
                    std::cout<<"   ";
            //    std::cout<<"-";
            }
            std::cout<<std::endl;
        }

        uint32_t closest;

        map< uint32_t , set < uint32_t > >::iterator it4;
        map< uint32_t , set < uint32_t > >::iterator it3;
        for(it4 = graph.begin(); it4 != graph.end(); it4++)
        {
            closest=closestUnvisited();
            remember[closest]=1;
            for(it3 = graph.begin(); it3 != graph.end(); it3++)
            {
                if(!remember[it3->first] && closest!=it3->first)
                {
                    if(apostasi[it3->first] > apostasi[closest] + matrix[closest][it3->first])
                    {
                        apostasi[it3->first]=apostasi[closest] + matrix[closest][it3->first];
                        progonos[it3->first]=closest;
                        std::cout<<"writting to progonos------------------------"<<std::endl;
                    }
                }
            }
        }

        results();

        /*delete progonos;
        delete apostasi;
        delete remember;*/
    }

    uint32_t Graph::closestUnvisited()
    {
        uint32_t min=100000;
        uint32_t closest;
        map< uint32_t , set < uint32_t > >::iterator it;
        for(it = graph.begin(); it != graph.end(); it++)
        {
            if(!remember[it->first] && (min>=apostasi[it->first]) )
            {
                min=apostasi[it->first];
                closest=it->first;
            }
        }

        return closest;
    }


    void Graph::results()
    {
        map< uint32_t , set < uint32_t > >::iterator it;
        for(it = graph.begin(); it != graph.end(); it++)
        {
            if(it->first!=source)
            {
                if(progonos[it->first]==-1)
                {
                    std::cout<<it->first<<" is unreachable from "<<source<<std::endl;
                }
                else
                {
                    std::cout<<it->first<<"---"<<std::endl;
                }

                std::cout<<"---"<<apostasi[it->first]<<std::endl;
            }
        }
    }
}
