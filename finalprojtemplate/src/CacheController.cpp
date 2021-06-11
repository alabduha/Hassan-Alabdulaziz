/*
	Cache Simulator (Starter Code) by Justin Goins
	Oregon State University
	Spring Term 2021
*/

#include "CacheController.h"
#include <iostream>
#include <fstream>
#include <regex>
#include <cmath>
#include <vector>
#include <time.h>
#include <stdlib.h>

using namespace std;

CacheController::CacheController(CacheInfo ci, string tracefile) {
	// store the configuration info
	this->ci = ci;
	this->inputFile = tracefile;
	this->outputFile = this->inputFile + ".out";
	// compute the other cache parameters
	this->ci.numByteOffsetBits = log2(ci.blockSize);
	this->ci.numSetIndexBits = log2(ci.numberSets);
	// initialize the counters
	this->globalCycles = 0;
	this->globalHits = 0;
	this->globalMisses = 0;
	this->globalEvictions = 0;

    //srand(time(0));
	// create your cache structure
	// ...

    Cache = new AddressInfo*[ci.numberSets];

    for (unsigned long int i=0; i<ci.numberSets; i++){
        Cache[i] = new AddressInfo[ci.associativity];

    }

	// manual test code to see if the cache is behaving properly
	// will need to be changed slightly to match the function prototype
	/*
	cacheAccess(false, 0);
	cacheAccess(false, 128);
	cacheAccess(false, 256);

	cacheAccess(false, 0);
	cacheAccess(false, 128);
	cacheAccess(false, 256);
	*/
}

/*
	Starts reading the tracefile and processing memory operations.
*/
void CacheController::runTracefile() {
	cout << "Input tracefile: " << inputFile << endl;
	cout << "Output file name: " << outputFile << endl;

	// process each input line
	string line;
    //count writes
    int writes = 0;
    //count reads
    int reads = 0;
	// define regular expressions that are used to locate commands
	regex commentPattern("==.*");
	regex instructionPattern("I .*");
	regex loadPattern(" (L )(.*)(,)([[:digit:]]+)$");
	regex storePattern(" (S )(.*)(,)([[:digit:]]+)$");
	regex modifyPattern(" (M )(.*)(,)([[:digit:]]+)$");

	// open the output file
	ofstream outfile(outputFile);
	// open the output file
	ifstream infile(inputFile);
	// parse each line of the file and look for commands
	while (getline(infile, line)) {
		// these strings will be used in the file output
		string opString, activityString;
		smatch match; // will eventually hold the hexadecimal address string
		unsigned long int address;
		// create a struct to track cache responses
		CacheResponse response;

        response.hits = false;
        response.evictions = false;
        response.misses = false;
        response.dirtyEvictions = false;
		// ignore comments
		if (std::regex_match(line, commentPattern) || std::regex_match(line, instructionPattern)) {
			// skip over comments and CPU instructions
			continue;

            //load
		} else if (std::regex_match(line, match, loadPattern)) {
			cout << "Found a load op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(&response, false, address, stoi(match.str(4)));
			logEntry(outfile, &response);

            //increment reads
            reads++;

            //store
		} else if (std::regex_match(line, match, storePattern)) {
			cout << "Found a store op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(&response, true, address, stoi(match.str(4)));
			logEntry(outfile, &response);

            //inc writes
            writes++;

            //modify
		} else if (std::regex_match(line, match, modifyPattern)) {
			cout << "Found a modify op!" << endl;
			istringstream hexStream(match.str(2));
			// first process the read operation
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(&response, false, address, stoi(match.str(4)));
			logEntry(outfile, &response);
			outfile << endl;

            reads++;

			// now process the write operation
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(&response, true, address, stoi(match.str(4)));
			logEntry(outfile, &response);


            writes++;

            //error
		} else {
			throw runtime_error("Encountered unknown line format in tracefile.");
		}
		outfile << endl;
	}
	// add the final cache statistics
	outfile << "Hits: " << globalHits << " Misses: " << globalMisses << " Evictions: " << globalEvictions << endl;
	outfile << "Cycles: " << globalCycles << " Reads:" << reads << " Writes:" << writes<< endl;

	infile.close();
	outfile.close();

    //delete the cache array
    for (unsigned long int i = 0; i<ci.numberSets; i++){
        delete[] Cache[i];
    }
    delete[] Cache;

}

/*
	Report the results of a memory access operation.
*/
void CacheController::logEntry(ofstream& outfile, CacheResponse* response) {

   // outfile << " " << response->cycles << " L1" << (response->hits ? " hit" : " miss") << (response->evictions ? " eviction" : "");

    outfile << " " << response->cycles;

    if (response->hits > 0)
		outfile << " hit";
	if (response->misses > 0)
		outfile << " miss";
	if (response->evictions > 0)
		outfile << " eviction";
}

/*
	Calculate the block index and tag for a specified address.
*/
CacheController::AddressInfo CacheController::getAddressInfo(unsigned long int address) {
	AddressInfo ai;
	// this code should be changed to assign the proper index and tag
    ai.tag = address >> ci.numSetIndexBits >> ci.numByteOffsetBits;
    ai.setIndex = (address - (ai.tag << ci.numByteOffsetBits << ci.numSetIndexBits)) >> ci.numByteOffsetBits;

	return ai;
}

/*
	This function allows us to read or write to the cache.
	The read or write is indicated by isWrite.
	address is the initial memory address
	numByte is the number of bytes involved in the access
*/
void CacheController::cacheAccess(CacheResponse* response, bool isWrite, unsigned long int address, int numBytes) {
	// determine the index and tag
	AddressInfo ai = getAddressInfo(address);

	cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;


     //Search for matching tag within Index

    response->hits=false;
    response->misses=false;
    response->evictions=false;

    unsigned int count=0;
    unsigned int count2=0;

    while(count < ci.associativity) {
        if ((Cache[ai.setIndex][count].validBit == 1) && (Cache[ai.setIndex][count].tag == ai.tag)){
            //Update LRUcounters
            if (ci.rp == ReplacementPolicy::LRU) {

                while (count2<ci.associativity){
                    if(Cache[ai.setIndex][count2].validBit == 1){
                        Cache[ai.setIndex][count2].LRU_data++;
                    }
                    count2++;
                }
                Cache[ai.setIndex][count].LRU_data = 0;
            }

            //Update Dirty Bit
            if ((isWrite) && (ci.wp == WritePolicy::WriteBack)){
                Cache[ai.setIndex][count].dirtyBit = 1;
            }
            response->hits = true;
        }
        count++;
    }


    count=-1;
    count2=0;

   // response->misses = true;
    if (!response->hits){
         bool Empty = false;


         do {


             count++;
             //Check for empty block
             if (Cache[ai.setIndex][count].validBit == 0) {
                 Cache[ai.setIndex][count].validBit = 1;
                 Cache[ai.setIndex][count].tag = ai.tag;
                 Cache[ai.setIndex][count].setIndex = ai.setIndex;

                 Empty = true;

                 //response->misses = true;

                //Update LRUcounters
                if(ci.rp == ReplacementPolicy::LRU) {
                    while (count2<ci.associativity){
                        if (Cache[ai.setIndex][count2].validBit == 1){
                            Cache[ai.setIndex][count2].LRU_data++;
                        }
                        count2++;
                    }

                    Cache[ai.setIndex][count].LRU_data = 0;
                }

                //Update Dirty Bit
                if ((isWrite) && (ci.wp == WritePolicy::WriteBack)){
                    Cache[ai.setIndex][count].dirtyBit = 1;
                }

                break;
            }
        }while(count<ci.associativity);

         count2=0;
        //Use proper Replacement Policy
        if (!Empty){

            //LRU
            if (ci.rp == ReplacementPolicy::LRU) {
                unsigned int leastUsed = 0;
                unsigned int Counter3 = 0;

                //Find least used
                for (unsigned int i=0; i < ci.associativity; i++){
                    if (Cache[ai.setIndex][i].LRU_data > Counter3){
                        Counter3 = Cache[ai.setIndex][i].LRU_data;
                        leastUsed = i;
                    }
                }

                Cache[ai.setIndex][leastUsed].tag = ai.tag;

                //Update LRUcounters

                if (ci.rp == ReplacementPolicy::LRU) {
                    while (count2<ci.associativity){

                         if (Cache[ai.setIndex][count2].validBit == 1){
                             Cache[ai.setIndex][count2].LRU_data++;
                            }
                         count2++;
                    }
                }

                Cache[ai.setIndex][leastUsed].LRU_data = 0;

                //Check Dirty Bit
                if ((ci.wp == WritePolicy::WriteBack)&&(Cache[ai.setIndex][leastUsed].dirtyBit == 1)){
                    response->dirtyEvictions = true;

                    //Reset dirty bit
                    if (!isWrite){
                        Cache[ai.setIndex][leastUsed].dirtyBit = 0;
                    }

                }
                response->misses = true;
                response->evictions = true;

            }
            //Random
            else {
                unsigned int rand_num = rand() % ci.associativity;
                Cache[ai.setIndex][rand_num].tag = ai.tag;

                //Check Dirty Bit
                if ((ci.wp == WritePolicy::WriteBack) && (Cache[ai.setIndex][rand_num].dirtyBit == 1)){
                        response->dirtyEvictions = true;

                        //Reset dirty bit
                        if (!isWrite){
                            Cache[ai.setIndex][rand_num].dirtyBit = 0;
                        }

                }
                response->misses = true;
                response->evictions = true;
            }
        }
    }

    if(!response->hits){
        response->misses=true;
    }

	// your code should also calculate the proper number of cycles that were used for the operation
	response->cycles = 0;

    if(response->hits){ // just the cache access
        response->cycles = ci.cacheAccessCycles;

        if(isWrite && ci.wp == WritePolicy::WriteThrough){
            response->cycles += ci.memoryAccessCycles;
        }

    }

    else { // miss
        response->cycles = (ci.cacheAccessCycles + ci.memoryAccessCycles);

        if(response->dirtyEvictions){
            response->cycles += (ci.cacheAccessCycles + ci.memoryAccessCycles);
        }
    }

    globalCycles += response->cycles;
	// your code needs to update the global counters that track the number of hits, misses, and evictions

    if (response->hits){
        globalHits++;

    }
    if (response->misses){
        globalMisses++;
    }

    if(response->evictions) {
        globalEvictions++;
    }

	if (response->hits > 0)
		cout << "Operation at address " << std::hex << address << " caused " << response->hits << " hit(s)." << std::dec << endl;
	if (response->misses > 0)
		cout << "Operation at address " << std::hex << address << " caused " << response->misses << " miss(es)." << std::dec << endl;

	cout << "-----------------------------------------" << endl;

	return;
}

