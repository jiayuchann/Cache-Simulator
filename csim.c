#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <error.h>
#include <stdbool.h>

// used for timestamp
struct timespec tv;

typedef struct Line{
	int valid;
	unsigned long long tag;
    // for implementing FIFO policy
    double timestamp;
    // block bits not really needed
}Line;

typedef struct Set{
	Line *lines;
}Set;

typedef struct Cache{
	Set *sets;
}Cache;

int hitCount = 0;
int missCount = 0;
int evictionCount = 0;

// reference: https://stackoverflow.com/questions/23092307/cache-simulator, they implemented the LRU policy, 
// but for this assignment we are using FIFO policy
int main(int argc, char *argv[]){

    // Usage
    if (argc != 9){
        printf("Usage: ./csim -s <s> -E <E> -b <b> -t <tracefile>\n");
        exit(1);
    }

    int setIndexBits;
	int linesPerSet;
	int blockBits;
    char* tracefile;

    for (int i = 0; i < argc; i++){
        if (strcmp(argv[i], "-s") == 0){
            setIndexBits = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-E") == 0){
            linesPerSet = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-b") == 0){
            blockBits = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-t") == 0){
            tracefile = argv[i+1];
        }
    }

    //set up cache
    Cache cache;

    unsigned long long numberOfSets = 1 << setIndexBits;

    cache.sets = calloc(numberOfSets, sizeof(Set));
    for (int i = 0; i < numberOfSets; i++){
        cache.sets[i].lines = calloc(linesPerSet, sizeof(Line));
    }

    FILE *inputfile = fopen(tracefile, "r");
    char operation;
    unsigned long long address;
    int size;

    while (fscanf(inputfile, " %c %llx,%d", &operation, &address, &size) != EOF){

        // M will always have a hit
        if (operation == 'M'){
            hitCount++;
        }
        
        // ignore I 
        if (operation == 'L' || operation == 'S' || operation == 'M'){
            // structure: TTTTTTTTTTTIIIIIIIIIBBBBBB (64 bit address)
            int tagBits = 64 - setIndexBits - blockBits;
            // TTTTTTTTTTTTTIIIIIIIBBBBBB => TTTTTTTTTTTTT
            unsigned long long tag = address >> (64 - tagBits);
            // TTTTTTTTTTTTTTTTTTIIIIIIBBBB => IIIIIIBBBB000000000000000000 => IIIIII
            unsigned long long setIndex = (address << tagBits) >> (tagBits + blockBits); 
            
            bool hit = false;
            bool emptySpot = false;
            int emptySpotIndex = 0;

            for (int i = 0; i < linesPerSet; i++){
                // if it's valid and tag matches, it's a hit
                if (cache.sets[setIndex].lines[i].valid){
                    if (cache.sets[setIndex].lines[i].tag == tag){
                        hitCount++;
                        hit = true;
                    }
                // if not valid, it would be an empty spot
                }else{
                    emptySpotIndex = i;
                    emptySpot = true;
                }
            }

            // if it is a miss
            if (!hit){
                missCount++;

                // if there is an empty spot, use it
                if (emptySpot){
                    cache.sets[setIndex].lines[emptySpotIndex].tag = tag;
                    cache.sets[setIndex].lines[emptySpotIndex].valid = 1;

                    // reference: https://lloydrochester.com/post/c/c-timestamp-epoch/
                    // this is to update the timestamp
                    char time_str[32];
                    clock_gettime(CLOCK_REALTIME, &tv);
                    sprintf(time_str, "%ld.%.9ld\n", tv.tv_sec, tv.tv_nsec);
                    cache.sets[setIndex].lines[emptySpotIndex].timestamp = atof(time_str);

                }else{ // otherwise, do the eviction
                    evictionCount++;

                    // within the same setIndex, search for the smallest timestamp, and that index will be the eviction index
                    double minTime = cache.sets[setIndex].lines[0].timestamp;
                    int evictionIndex = 0;

                    for (int i = 1; i < linesPerSet; i++){
                        if (cache.sets[setIndex].lines[i].timestamp < minTime){
                            minTime = cache.sets[setIndex].lines[i].timestamp;
                            evictionIndex = i;
                        }
                    }

                    cache.sets[setIndex].lines[evictionIndex].tag = tag;

                    // update the timestamp
                    char time_str[32];
                    clock_gettime(CLOCK_REALTIME, &tv);
                    sprintf(time_str, "%ld.%.9ld\n", tv.tv_sec, tv.tv_nsec);
                    cache.sets[setIndex].lines[evictionIndex].timestamp = atof(time_str);
                }
            }
        }// end of big if statement

    }

    // print results
    printf("hits:%d misses:%d evictions:%d\n", hitCount, missCount, evictionCount);


    // memory freeing
    fclose(inputfile);
    for (int i = 0; i < numberOfSets; i++){
        free(cache.sets[i].lines);
    }
    free(cache.sets);
    
    return 0;
}
