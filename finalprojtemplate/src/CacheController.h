/*
	Cache Simulator (Starter Code) by Justin Goins
	Oregon State University
	Spring Term 2021
*/

#ifndef _CACHECONTROLLER_H_
#define _CACHECONTROLLER_H_

#include "CacheStuff.h"
#include <string>
#include <fstream>

class CacheController {
	private:
		struct AddressInfo {
			unsigned long int tag;
			unsigned int setIndex;
            unsigned int validBit;
            unsigned int LRU_data;
            unsigned int dirtyBit;
		};
		unsigned int globalCycles;
		unsigned int globalHits;
		unsigned int globalMisses;
		unsigned int globalEvictions;
		std::string inputFile, outputFile;

		CacheInfo ci;

        AddressInfo** Cache;


		// function to allow read or write access to the cache
		void cacheAccess(CacheResponse*, bool, unsigned long int, int);
		// function that can compute the index and tag matching a specific address
		AddressInfo getAddressInfo(unsigned long int);
		// function to add entry into output file
		void logEntry(std::ofstream&, CacheResponse*);


	public:
		CacheController(CacheInfo, std::string);
		void runTracefile();
};

#endif //CACHECONTROLLER
