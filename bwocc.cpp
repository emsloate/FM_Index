//
//  bwocc.cpp
//  
//
//  Created by Elliott Sloate on 10/22/20.
//

#include "bwocc.hpp"
//#include "bwt.hpp"
#include "BWT/BWT/BWT/BWT.h"
#include "../rank_support/rank_support.hpp"
#include "../select_support/select_support.hpp"
#include <iostream>
#include <math.h>
#include <map>
#include <typeinfo>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <sstream>
#include <time.h>
#include <unistd.h>
#include <chrono>
#include <boost/dynamic_bitset.hpp>
#include <filesystem>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost;

class FM_Index{
    public:
    int * first_col;
    string bwt_string;
    map<char,rank_support*> occ;
    map<char,int> alphabet;
    
    FM_Index(){}
    ~FM_Index(){
        occ.erase(occ.begin(),occ.end());
    }
    FM_Index(string input){
        //input: string input
        //output: a FM_Index object
        char k;
        map<char,dynamic_bitset<>*> occ_bs;
        //buld bwt. makePrintable adds terminal char ^ to bwt, which we need
        //cout << "Creating BWT: " << endl;
        bwt_string = BWT_encode(input);
        //bwt_string = makePrintable(bwt(input));
        //cout << "Creating Index vars" << endl;
        // number of occurences of each char in the string
        // we can loop through bwt string, since it has the same chars as the input
        for (int i = 0;i<bwt_string.size();i++){
            k = bwt_string[i];
            if (k >= 65 && k <= 122){
                alphabet[k] ++;
                //for occ table, initialize a rank support with bitvector size input.size() if we have not already
                if( occ_bs.count(k) == 0){
                    occ_bs[k] = new dynamic_bitset<>(bwt_string.size(),0);
                    occ_bs[k]->set(i,true);
                }
                else{
                    //we already have a dynamic_bitset, still need to set this spot = 1
                    occ_bs[k]->set(i,true);
                }
            }
        }
        //create array that stores cummulative count of alphabet up to character, in order
        first_col = new int[alphabet.size()];
        int index = 0;
        int count = 0;
        //this loops through the keys in order - yay
        for(map<char,int>::iterator iter = alphabet.begin(); iter != alphabet.end(); ++iter){
            //put cummulative count at correct spot in first_col
            count = count + iter->second;
            first_col[index] = count;
            
            //now just have map save order of the keys for looking up later
            k = iter->first;
            alphabet[k] = index;
            index++;
            
        }
        //build occ table from occ_bs
        for(map<char,dynamic_bitset<>* >::iterator iter = occ_bs.begin(); iter != occ_bs.end(); ++iter){
            //put cummulative count at correct spot in first_col
            k = iter->first;
            dynamic_bitset<> * b = iter->second;
            occ[k] = new rank_support(b);
        }
        //cout << "Built FM-Index" << endl;

    }
    void save(string& file){
        std::ofstream myfile;
        myfile.open(file);
        //write size of alphabet, first_col
        myfile << alphabet.size() << endl;
        //write first_col
        for(int i = 0; i < alphabet.size(); i++){
            myfile << first_col[i] << "," ;
        }
        myfile << endl;
        //write bwt string
        myfile << bwt_string << endl;
        
        //write alphabet
        map<char,int>::iterator it;
        for ( it = alphabet.begin(); it != alphabet.end(); it++ )
        {
            //char key
            char k = it->first;
            // position in first_col
            int pos = it->second;
            //write k,v on one line
            myfile << k << ',' << pos << endl;

        }
        //write occ
        //lazy idea: write each rank_support in occ to a file, write file name in line
        map<char,rank_support*>::iterator it2;
        for ( it2 = occ.begin(); it2 != occ.end(); it2++ )
        {
            //char key
            char k = it2->first;
            // rank_support
            rank_support * r = it2->second;
            string filename = file.substr(0, file.find(".txt"));
            filename.append("_occ_col_");
            filename.push_back(k);
            filename.append(".txt");
            //write k,v on one line
            myfile << k << ',' << filename << endl;
            r->save(filename);
        }
        myfile.close();
        
    }
    void load(string& fname){
        string line;
        std::ifstream myfile (fname);
        stringstream buffer;
        //read file
        if (myfile.is_open()){
            buffer << myfile.rdbuf();
            getline(buffer,line);
            //size of alphabet
            int sigma = stoi(line);
            first_col = new int[sigma];
            getline(buffer,line);
            //read values of first_col
            int i = 0;
            stringstream linestream;
            linestream.str(line);
            string tok;
            while(getline(linestream, tok, ',')){
                if (tok.size() > 0){
                   first_col[i] = stoi(tok);
                   i++;
                }
            }
            i = 0;
            //read bwt string
            getline(buffer,line);
            bwt_string = line;
            //read alphabet map
            for(int j = 0;j<sigma;j++){
                getline(buffer,line);
                //inner loop reads contents of each line
                linestream.clear();
                linestream.str(line);
                char k;
                int pos;
                while(getline(linestream, tok, ',')){
                    //char key
                    if (i == 0){
                        k = tok[0];
                        i++;
                    }else if(i == 1){   //position value, int
                        pos = stoi(tok);
                    }
                }
                //add to map
                alphabet[k] = pos;
                i = 0;
            }
            
            //read occ map
            for(int j = 0;j<sigma;j++){
                getline(buffer,line);
                //inner loop reads contents of each line
                linestream.clear();
                linestream.str(line);
                char k;
                string occfile;
                rank_support * rnk = new rank_support();
                while(getline(linestream, tok, ',')){
                    //char key
                    if (i == 0){
                        k = tok[0];
                        i++;
                    }else if(i == 1){   //string value, filename where occ rank_support is saved
                        occfile = tok;
                        rnk->load(occfile);
                    }
                }
                //add to map
                occ[k] = rnk;
                i = 0;
            }
            
        }
        
        myfile.close();
    }
    pair<int,int> search(string query){
        //starting from end of query, perform FM search
        char q;
        char next;
        int start_first;
        int end_first;
        //our first ranks
        int rank_beg = 1;
        //this is the first char to look at
        char fq = query[query.size()-1];
        //to get # of times we have seen char, calc first_col[alphabet[fq]] - first_col[alphabet[fq] - 1]
        int rank_end;
        if(occ.count(fq) == 0){
            return pair<int,int>(0,0);
        }
        if (alphabet[fq] == 0){
            rank_end = first_col[alphabet[fq]];
        }else{
            rank_end = first_col[alphabet[fq]] - first_col[alphabet[fq] - 1];
        }
        //cout << "starting rank to look for: " << rank_beg << "," << rank_end << endl;
        
        //occ table stuff
        int start_rank;
        int end_rank;
        for(int i = query.size()-1;i > 0;i--){
            //character in query
            q = query[i];
            //the starting point in the first col for that character - 1
            if (alphabet[q] == 0){
                start_first = 0;
                //if we want to start at rank 4, then add 4
                //subtract 1 b/c we want to look one position before, so we know if the first rank we see is new or not
                end_first = rank_end;
                //if we want to end at rank 6, then simply add 6 to our starting position
                start_first = start_first + rank_beg - 1;
            }
            //the sum in first_col[alphabet[q] - 1] is the number of chars in the string before our char
            else{
                start_first = first_col[alphabet[q] - 1];
                end_first = start_first + rank_end;
                start_first = start_first + rank_beg - 1;
            }
            
            
            //query the occ table for the next char
            next = query[i-1];
            //cout << "Looking from position " << start_first << " to position " << end_first << " in occ" << endl;
            //cout << "Looking for " << next << endl;
            rank_support * r = occ[next];
            start_rank = r->rank1(start_first);
            end_rank = r->rank1(end_first);
            //no characters matching next found in the occ table
            if (start_rank == end_rank){
                return pair<int,int>(start_first,start_first);
            }
            else{
                //what ranks did we find?, the first rank we found would
                rank_beg = start_rank+1;
                rank_end = end_rank;
            }
            //cout << "Found " << end_rank-start_rank << " valid chars" << endl;
            //cout << "Looking from rank " << rank_beg << " of " << next << " to rank " << rank_end << endl;
            
        }
        //if we have found the entire query, we should exit loop
        //have to get positions in first column correct still
        if (alphabet[next] == 0){
            start_first = rank_beg;
            end_first = rank_end + 1;
        }
        else{
            start_first = first_col[alphabet[next] - 1];
            end_first = start_first + rank_end + 1;
            start_first = start_first + rank_beg;
        }
        //don't subtract 1 b/c we want to start where the letter is, not 1 before
        
        return pair<int,int>(start_first,end_first);
        
        
    }
    void multqueries(string& fname,int numq){
        //inputs: fname - name of file,numq = number of queries to run, if 0 as input, read whole file
        string line;
        cout << "File: " << fname << endl;
        ifstream myfile (fname);
        pair<int,int> output;
        if (numq == 0){
            if (myfile.is_open()){
                cout << "Making queries" << endl;
                while(getline(myfile,line)){
                    output = search(line);
                    cout << output.first << "\t" << output.second << endl;
                }
            }
        }else{
            if (myfile.is_open()){
                while(getline(myfile,line) && (numq > 0)){
                    output = search(line);
                    cout << output.first << "\t" << output.second << endl;
                    numq--;
                }
            }
            
        }
        myfile.close();
    }
    
    
    
};

string readfile(string& fname){
    string line;
    ifstream myfile (fname);
    stringstream buffer;
    if (myfile.is_open()){
        getline(myfile,line);
    }
    myfile.close();
    return line;
}
int main(int argc, char *argv[]){
    
    string index;
    const char * input;
    //command - either build or query
    if(argc > 1){
        string command = argv[1];
        string arg2 = argv[2];
        string arg3 = argv[3];
        if (command == "build"){
            index = readfile(arg2);
            FM_Index f(index);
            f.save(arg3);
            cout << f.alphabet.size() << endl;
            cout << f.bwt_string.size() << endl;
        } else if(command == "query"){
            FM_Index f;
            f.load(arg2);
            f.multqueries(arg3,0);
            
        }
    }else {
        cout << "Benchmark testing..." << endl;
        string sequences[8] = {"GCF_000008665.1_ASM866v1_genomic","GCF_000800805.1_ASM80080v1_genomic",    "GCF_002156965.1_ASM215696v1_genomic", "GCF_002844195.1_ASM284419v1_genomic",
            "GCF_000092305.1_ASM9230v1_genomic", "GCF_001633105.1_ASM163310v1_genomic",    "GCF_002214385.1_ASM221438v1_genomic", "GCF_003434225.1_ASM343422v1_genomic"};
        map<string,string *> files_needed;
        string seqfile;
        string realq;
        string fakeq;
        string seq;
        string results = "results.txt";
        string results2 = "real_fake.txt";
        std::ofstream myfile;
        std::ofstream myfile2;
        
        for(int i = 0;i<8;i++){
            seqfile = "./sequences/" + sequences[i] + ".txt";
            realq = "./queries/" + sequences[i] + "_real.txt";
            fakeq = "./queries/" + sequences[i] + "_fake.txt";
            seq = readfile(seqfile);
            //build FM index
            FM_Index f(seq);
            int numqs[4] = {1000,10000,100000,1000000};
            uint64_t start;
            uint64_t end;
            uint64_t time;
            //time to do numqs[j] amount of queries
            write size of file, # of realq, # of fake q
            myfile.open(results,ios_base::app);
            for (int j = 0;j<4;j++){
               start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                //do queries
                f.multqueries(realq,numqs[j]);
                end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                time= end-start;
                myfile << sequences[i] << "," << seq.size() << "," << numqs[j] << "," << time << endl;
            }
            myfile.close();
            
            //doing real/fake query testing
            myfile2.open(results2,ios_base::app);
            float percreal[6] = {1.0,0.8,0.6,0.4,0.2,0.0};
            for(int j = 0;j<6;j++){
                start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                //do queries
                f.multqueries(realq,percreal[j] * 10000);
                f.multqueries(fakeq,(1 - percreal[j]) * 10000);
                end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                time = end-start;
                //write percentage real, time to line of this query.
                myfile2 << sequences[i] << "," << percreal[j] << "," << time << endl;
            }
            myfile2.close();

        }
        

        
        
    }
    return 0;
}
