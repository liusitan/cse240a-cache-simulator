//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
//
// TODO:Student Information
//
const char *studentName = "Sitan Liu";
const char *studentID = "A53306512";
const char *email = "sil017@ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;    // Number of sets in the I$
uint32_t icacheAssoc;   // Associativity of the I$
uint32_t icacheHitTime; // Hit Time of the I$

uint32_t dcacheSets;    // Number of sets in the D$
uint32_t dcacheAssoc;   // Associativity of the D$
uint32_t dcacheHitTime; // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize; // Block/Line size
uint32_t memspeed;  // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;      // I$ references
uint64_t icacheMisses;    // I$ misses
uint64_t icachePenalties; // I$ penalties

uint64_t dcacheRefs;      // D$ references
uint64_t dcacheMisses;    // D$ misses
uint64_t dcachePenalties; // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//
typedef struct parsed_address
{
  uint32_t tag;
  uint32_t index;
  uint32_t offset;
} parsed_address;

typedef struct cacheline
{
  uint32_t tag;
  // uint8_t* data;
} cacheline;

cacheline create_cacheline()
{
  cacheline res;
  
  res.tag = 4294967295U;
  return res;
};
typedef struct set
{
  cacheline *cls; // pointer to the set
  uint64_t *rec_bit;
  uint64_t asso;
} set;
set create_dset()
{
  set s;
  s.rec_bit = calloc(dcacheAssoc,sizeof(uint64_t) * dcacheAssoc);
  s.cls = calloc(dcacheAssoc,sizeof(cacheline) * dcacheAssoc);  for(int i = 0; i < dcacheAssoc; i++){
    s.cls[i].tag = 4294967295U; 
  }
  s.asso = dcacheAssoc;
  return s;
}
set create_iset()
{
  set s ;
  s.rec_bit = calloc(icacheAssoc,sizeof(uint64_t));
  s.cls = calloc(icacheAssoc,sizeof(cacheline));
  for(int i = 0; i < icacheAssoc; i++){
    s.cls[i].tag = 4294967295U; 
  }
  s.asso = icacheAssoc;
  return s;
}
set create_l2set()
{
  set s;
  s.rec_bit = calloc(l2cacheAssoc,sizeof(uint64_t) );
  s.cls = calloc(l2cacheAssoc,sizeof(cacheline));
    for(int i = 0; i < l2cacheAssoc; i++){
    s.cls[i].tag = 4294967295U; 
  }
  s.asso = l2cacheAssoc;
  return s;
}
typedef struct cache
{
  set *sets;
} cache;
cache icache;
cache dcache;
cache l2cache;

void insert_to_set(set *s, parsed_address pa)
{
  uint64_t l_rec = s->rec_bit[0];
  uint64_t lrec_id = 0;
  uint64_t h_rec = s->rec_bit[0];
  uint64_t hrec_id = 0;
  // get the lru  lrec is the lru
  for (int i = 0; i < s->asso; i++)
  {
    uint64_t rec = s->rec_bit[i];
    if (rec <= l_rec)
    {
      l_rec = rec;
      lrec_id = i;
    }
    if (rec > h_rec)
    {
      h_rec = rec;
      hrec_id = i;
    }
  }
  // replace the lru with the current one
  cacheline tobe_insert;
  tobe_insert.tag = pa.tag;
  s->cls[lrec_id] = tobe_insert;
  // the highest one + 1
  s->rec_bit[lrec_id] = h_rec + 1;
}

parsed_address parse_address(uint32_t address, uint32_t block_size, uint32_t set_size)
{
  uint32_t blockoffset = address % block_size;
  uint32_t setid = (address / block_size) % set_size;
  uint32_t tag = ((address / (block_size)) / set_size);
  parsed_address res;

  res.tag = tag;
  res.offset = blockoffset;
  res.index = setid;
  return res;
}

//
// TODO: Add your Cache data structures here
//

//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void init_icache()
{
  cache *s = &icache;
  s->sets = calloc(icacheSets, sizeof(set) );
  for (int i = 0; i < icacheSets; i++)
  {
    s->sets[i] = create_iset();
  }
}
void init_dcache()
{

  cache *s = &dcache;
  s->sets = calloc(dcacheSets,sizeof(set));
  for (int i = 0; i < dcacheSets; i++)
  {
    s->sets[i] = create_dset();
  }
}
void init_l2cache()
{
  cache *s = &l2cache;
  s->sets =calloc(l2cacheSets,sizeof(set));
  for (int i = 0; i < l2cacheSets; i++)
  {
    s->sets[i] = create_l2set();
  }
}
void init_cache()
{
  // Initialize cache stats
  icacheRefs = 0;
  icacheMisses = 0;
  icachePenalties = 0;
  dcacheRefs = 0;
  dcacheMisses = 0;
  dcachePenalties = 0;
  l2cacheRefs = 0;
  l2cacheMisses = 0;
  l2cachePenalties = 0;

  //
  // TODO: Initialize Cache Simulator Data Structures
  //
  init_icache();
  init_dcache();
  init_l2cache();
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
  //
  // TODO: Implement I$
  //
  // check if the has this tag;
  icacheRefs++;

  parsed_address pa = parse_address(addr, blocksize, icacheSets);
  set *s = &(icache.sets[pa.index]);
  int hit_id = -1;
  for (int i = 0; i < icacheAssoc; i++)
  {
    if (s->cls[i].tag == pa.tag)
    {

      // printf("addr: %x,extracted tag:%x, line tag:%x\n",addr, pa.tag,(s->cls[i].tag));
      hit_id = i;
      break;
    }
  }
  if ((hit_id)>=  0)
  {
    int32_t max = -1;
    for (int i = 0; i < icacheAssoc; i++)
    {
      max =
          max > s->rec_bit[i] ? max : s->rec_bit[i];
    }
    s->rec_bit[hit_id] = max + 1;
    return icacheHitTime;
  }
  else
  {
    // printf("------------------------miss--------------------------");
    icacheMisses++;
    insert_to_set(s, pa);
    uint32_t penalty = l2cache_access(addr); 
    icachePenalties += penalty;
    return icacheHitTime + penalty;
  }
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
  //
  // TODO: Implement D$
  dcacheRefs++;
  parsed_address pa = parse_address(addr, blocksize, dcacheSets);
  set *s = &(dcache.sets[pa.index]);
  int hit_id = -1;
  for (int i = 0; i < dcacheAssoc; i++)
  {
    if (s->cls[i].tag == pa.tag)
    {
      hit_id = i;
      break;
    }
  }
  if ((hit_id) >= 0)
  {
    uint32_t max = 0;
    for (int i = 0; i < dcacheAssoc; i++)
    {
      max =
            max > s->rec_bit[i] ? max : s->rec_bit[i];
    }
    s->rec_bit[hit_id] = max + 1;
    return dcacheHitTime;
  }
  else
  {
    dcacheMisses++;
    insert_to_set(s, pa);
    uint32_t penalty = l2cache_access(addr); 
    dcachePenalties += penalty;
    return dcacheHitTime + penalty;
  }
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
  l2cacheRefs++;
  parsed_address pa = parse_address(addr, blocksize, l2cacheSets);
  set *s = &(l2cache.sets[pa.index]);
  int hit_id = -1;
  for (int i = 0; i < l2cacheAssoc; i++)
  {
    if (s->cls[i].tag == pa.tag)
    {
      hit_id = i;
      break;
    }
  }
  if ((hit_id) >= 0)
  {

    uint32_t max = 0;
    for (int i = 0; i < l2cacheAssoc; i++)
    {
      max =
          max > s->rec_bit[i] ? max : s->rec_bit[i];
    }
    s->rec_bit[hit_id] = max + 1;
    return l2cacheHitTime;
  }
  else
  {
    l2cacheMisses++;
    insert_to_set(s, pa);
    l2cachePenalties += memspeed;

    return l2cacheHitTime + memspeed;
  }
}
