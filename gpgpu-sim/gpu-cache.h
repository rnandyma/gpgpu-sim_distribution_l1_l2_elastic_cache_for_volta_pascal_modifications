// Copyright (c) 2009-2011, Tor M. Aamodt, Tayler Hetherington
// The University of British Columbia
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution. Neither the name of
// The University of British Columbia nor the names of its contributors may be
// used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef GPU_CACHE_H
#define GPU_CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include "../abstract_hardware_model.h"
#include "../tr1_hash_map.h"
#include "gpu-misc.h"
#include "mem_fetch.h"

#include <iostream>
#include "addrdec.h"

#define MAX_DEFAULT_CACHE_SIZE_MULTIBLIER 4

enum cache_block_state { INVALID = 0, RESERVED, VALID, MODIFIED };

enum cache_request_status {
  HIT = 0,
  HIT_RESERVED,
  MISS,
  RESERVATION_FAIL,
  SECTOR_MISS,
  HIT_WORD_RESERVED,
  WORD_MISS,
  WORD_RESERVATION_FAIL,
  NUM_CACHE_REQUEST_STATUS
};

enum cache_reservation_fail_reason {
  LINE_ALLOC_FAIL = 0,  // all line are reserved
  MISS_QUEUE_FULL,      // MISS queue (i.e. interconnect or DRAM) is full
  MSHR_ENRTY_FAIL,
  MSHR_MERGE_ENRTY_FAIL,
  MSHR_RW_PENDING,
  WORD_ALLOC_FAIL,
  NUM_CACHE_RESERVATION_FAIL_STATUS
};

enum cache_event_type {
  WRITE_BACK_REQUEST_SENT,
  READ_REQUEST_SENT,
  WRITE_REQUEST_SENT,
  WRITE_ALLOCATE_SENT,
  WRITE_BACK_WORD_REQUEST_SENT,
  READ_WORD_REQUEST_SENT,
  WRITE_WORD_REQUEST_SENT,
  WRITE_WORD_ALLOCATE_SENT
};
// Elastic: Following struct evicted_word_info created for evicting a word from a line
struct evicted_word_info {
  new_addr_type m_addr;
  unsigned m_modified_size;
  evicted_word_info() {
    m_addr = 0;
    m_modified_size = 0;
  }
  void set_word_info(new_addr_type addr, unsigned modified_size) {
    m_addr = addr;
    m_modified_size = modified_size; //Elastic: Always 1 as we evict one 32 byte only
    assert((m_modified_size/32)==1);
  }
};


struct evicted_block_info {
  new_addr_type m_block_addr;
  unsigned m_modified_size;
  evicted_block_info() {
    m_block_addr = 0;
    m_modified_size = 0;
  }
  void set_info(new_addr_type block_addr, unsigned modified_size) {
    m_block_addr = block_addr;
    m_modified_size = modified_size;
  }
};

struct cache_event {
  enum cache_event_type m_cache_event_type;
  evicted_block_info m_evicted_block;  // if it was write_back event, fill the evicted block info
  evicted_word_info m_evicted_word;  // if it was write_back_word event, fill the
                                       // the evicted word info

  cache_event(enum cache_event_type m_cache_event) {
    m_cache_event_type = m_cache_event;
  }

  cache_event(enum cache_event_type cache_event,
              evicted_block_info evicted_block) {
    m_cache_event_type = cache_event;
    m_evicted_block = evicted_block;
  }
  cache_event(enum cache_event_type cache_event,
              evicted_word_info evicted_word) {
    m_cache_event_type = cache_event;
    m_evicted_word = evicted_word;
  }

};
//struct elastic_cache_event {
//  enum cache_event_type m_cache_event_type;
//  evicted_word_info m_evicted_word;  // if it was write_back event, fill the
//                                       // the evicted block info
//
//  elastic_cache_event(enum cache_event_type m_cache_event) {
//    m_cache_event_type = m_cache_event;
//  }
//
//  elastic_cache_event(enum cache_event_type cache_event,
//              evicted_word_info evicted_word) {
//    m_cache_event_type = cache_event;
//    m_evicted_word = evicted_word;
//  }
//};

const char *cache_request_status_str(enum cache_request_status status);

struct cache_block_t {
  cache_block_t() {
    m_tag = 0;
    m_block_addr = 0;
    m_common_tag = 0;    //Elastic: common tag init
    for(int i=0;i<4;i++)   //Elastic: Come back to configure later
    m_chunk_tag[i] = 0;    //Elastic: chunk tag init

  }

  virtual void allocate(new_addr_type tag, new_addr_type block_addr,
                        unsigned time,
                        mem_access_sector_mask_t sector_mask) = 0;
  virtual void fill(unsigned time, mem_access_sector_mask_t sector_mask){}

  virtual bool is_invalid_line() = 0;
  virtual bool is_valid_line() = 0;
  virtual bool is_reserved_line() = 0;
  virtual bool is_modified_line() = 0;

  virtual enum cache_block_state get_status(
      mem_access_sector_mask_t sector_mask){}
  virtual void set_status(enum cache_block_state m_status,
                          mem_access_sector_mask_t sector_mask){}

  virtual unsigned long long get_last_access_time() = 0;
  virtual void set_last_access_time(unsigned long long time,
                                    mem_access_sector_mask_t sector_mask) = 0;
  virtual unsigned long long get_alloc_time() = 0;
  virtual void set_ignore_on_fill(bool m_ignore,
                                  mem_access_sector_mask_t sector_mask) = 0;
  virtual void set_modified_on_fill(bool m_modified,
                                    mem_access_sector_mask_t sector_mask) = 0;
  virtual unsigned get_modified_size() = 0;
  virtual void set_m_readable(bool readable,
                              mem_access_sector_mask_t sector_mask) = 0;
  virtual bool is_readable(mem_access_sector_mask_t sector_mask) = 0;
  virtual void print_status() = 0;
  virtual ~cache_block_t() {}

  new_addr_type m_tag;
  new_addr_type m_block_addr;
  new_addr_type m_addr;
  new_addr_type m_chunk_tag[4]; //Elastic: Come back later to configure
  new_addr_type m_common_tag;
   virtual bool is_invalid_word(unsigned bidx){}
  virtual bool is_valid_word(unsigned bidx){}
  virtual bool is_reserved_word(unsigned bidx){}
  virtual bool is_modified_word(unsigned bidx){}
  virtual enum cache_block_state get_word_status(unsigned bidx){}
  virtual void set_word_status(enum cache_block_state m_status,unsigned bidx){}
  virtual unsigned long long get_word_last_access_time(unsigned bidx){}
  virtual void set_word_last_access_time(unsigned long long time,unsigned bidx){}
  virtual unsigned long long get_word_alloc_time(unsigned bidx){}
  virtual void set_word_ignore_on_fill(bool m_ignore,unsigned bidx){}
  virtual void set_m_word_readable(bool readable,unsigned bidx){}
  virtual bool is_word_readable(unsigned bidx){}
  virtual void elastic_fill(unsigned time, unsigned bidx){}

};

struct line_cache_block : public cache_block_t {
  line_cache_block() {
    m_alloc_time = 0;
    m_fill_time = 0;
    m_last_access_time = 0;
    m_status = INVALID;
    m_ignore_on_fill_status = false;
    m_set_modified_on_fill = false;
    m_readable = true;
  }
  void allocate(new_addr_type tag, new_addr_type block_addr, unsigned time,
                mem_access_sector_mask_t sector_mask) {
    m_tag = tag;
    m_block_addr = block_addr;
    m_alloc_time = time;
    m_last_access_time = time;
    m_fill_time = 0;
    m_status = RESERVED;
    m_ignore_on_fill_status = false;
    m_set_modified_on_fill = false;
  }
  void fill(unsigned time, mem_access_sector_mask_t sector_mask) {
    // if(!m_ignore_on_fill_status)
    //	assert( m_status == RESERVED );

    m_status = m_set_modified_on_fill ? MODIFIED : VALID;

    m_fill_time = time;
  }
  virtual bool is_invalid_line() { return m_status == INVALID; }
  virtual bool is_valid_line() { return m_status == VALID; }
  virtual bool is_reserved_line() { return m_status == RESERVED; }
  virtual bool is_modified_line() { return m_status == MODIFIED; }

  virtual enum cache_block_state get_status(
      mem_access_sector_mask_t sector_mask) {
    return m_status;
  }
  virtual void set_status(enum cache_block_state status,
                          mem_access_sector_mask_t sector_mask) {
    m_status = status;
  }
  virtual unsigned long long get_last_access_time() {
    return m_last_access_time;
  }
  virtual void set_last_access_time(unsigned long long time,
                                    mem_access_sector_mask_t sector_mask) {
    m_last_access_time = time;
  }
  virtual unsigned long long get_alloc_time() { return m_alloc_time; }
  virtual void set_ignore_on_fill(bool m_ignore,
                                  mem_access_sector_mask_t sector_mask) {
    m_ignore_on_fill_status = m_ignore;
  }
  virtual void set_modified_on_fill(bool m_modified,
                                    mem_access_sector_mask_t sector_mask) {
    m_set_modified_on_fill = m_modified;
  }
  virtual unsigned get_modified_size() {
    return SECTOR_CHUNCK_SIZE * SECTOR_SIZE;  // i.e. cache line size
  }
  virtual void set_m_readable(bool readable,
                              mem_access_sector_mask_t sector_mask) {
    m_readable = readable;
  }
  virtual bool is_readable(mem_access_sector_mask_t sector_mask) {
    return m_readable;
  }
  virtual void print_status() {
    printf("m_block_addr is %llu, status = %u\n", m_block_addr, m_status);
  }

 private:
  unsigned long long m_alloc_time;
  unsigned long long m_last_access_time;
  unsigned long long m_fill_time;
  cache_block_state m_status;
  bool m_ignore_on_fill_status;
  bool m_set_modified_on_fill;
  bool m_readable;
};

struct sector_cache_block : public cache_block_t {
  sector_cache_block() { init(); }

  void init() {
    for (unsigned i = 0; i < SECTOR_CHUNCK_SIZE; ++i) {
      m_sector_alloc_time[i] = 0;
      m_sector_fill_time[i] = 0;
      m_last_sector_access_time[i] = 0;
      m_status[i] = INVALID;
      m_ignore_on_fill_status[i] = false;
      m_set_modified_on_fill[i] = false;
      m_readable[i] = true;
    }
    m_line_alloc_time = 0;
    m_line_last_access_time = 0;
    m_line_fill_time = 0;
  }

  virtual void allocate(new_addr_type tag, new_addr_type block_addr,
                        unsigned time, mem_access_sector_mask_t sector_mask) {
    allocate_line(tag, block_addr, time, sector_mask);
  }

  void allocate_line(new_addr_type tag, new_addr_type block_addr, unsigned time,
                     mem_access_sector_mask_t sector_mask) {
    // allocate a new line
    // assert(m_block_addr != 0 && m_block_addr != block_addr);
    init();
    m_tag = tag;
    m_block_addr = block_addr;

    unsigned sidx = get_sector_index(sector_mask);

    // set sector stats
    m_sector_alloc_time[sidx] = time;
    m_last_sector_access_time[sidx] = time;
    m_sector_fill_time[sidx] = 0;
    m_status[sidx] = RESERVED;
    m_ignore_on_fill_status[sidx] = false;
    m_set_modified_on_fill[sidx] = false;

    // set line stats
    m_line_alloc_time = time;  // only set this for the first allocated sector
    m_line_last_access_time = time;
    m_line_fill_time = 0;
  }

  void allocate_sector(unsigned time, mem_access_sector_mask_t sector_mask) {
    // allocate invalid sector of this allocated valid line
    assert(is_valid_line());
    unsigned sidx = get_sector_index(sector_mask);

    // set sector stats
    m_sector_alloc_time[sidx] = time;
    m_last_sector_access_time[sidx] = time;
    m_sector_fill_time[sidx] = 0;
    if (m_status[sidx] == MODIFIED)  // this should be the case only for
                                     // fetch-on-write policy //TO DO
      m_set_modified_on_fill[sidx] = true;
    else
      m_set_modified_on_fill[sidx] = false;

    m_status[sidx] = RESERVED;
    m_ignore_on_fill_status[sidx] = false;
    // m_set_modified_on_fill[sidx] = false;
    m_readable[sidx] = true;

    // set line stats
    m_line_last_access_time = time;
    m_line_fill_time = 0;
  }

  virtual void fill(unsigned time, mem_access_sector_mask_t sector_mask) {
    unsigned sidx = get_sector_index(sector_mask);

    //	if(!m_ignore_on_fill_status[sidx])
    //	         assert( m_status[sidx] == RESERVED );

    m_status[sidx] = m_set_modified_on_fill[sidx] ? MODIFIED : VALID;

    m_sector_fill_time[sidx] = time;
    m_line_fill_time = time;
  }
  virtual bool is_invalid_line() {
    // all the sectors should be invalid
    for (unsigned i = 0; i < SECTOR_CHUNCK_SIZE; ++i) {
      if (m_status[i] != INVALID) return false;
    }
    return true;
  }
  virtual bool is_valid_line() { return !(is_invalid_line()); }
  virtual bool is_reserved_line() {
    // if any of the sector is reserved, then the line is reserved
    for (unsigned i = 0; i < SECTOR_CHUNCK_SIZE; ++i) {
      if (m_status[i] == RESERVED) return true;
    }
    return false;
  }
  virtual bool is_modified_line() {
    // if any of the sector is modified, then the line is modified
    for (unsigned i = 0; i < SECTOR_CHUNCK_SIZE; ++i) {
      if (m_status[i] == MODIFIED) return true;
    }
    return false;
  }

  virtual enum cache_block_state get_status(
      mem_access_sector_mask_t sector_mask) {
    unsigned sidx = get_sector_index(sector_mask);

    return m_status[sidx];
  }

  virtual void set_status(enum cache_block_state status,
                          mem_access_sector_mask_t sector_mask) {
    unsigned sidx = get_sector_index(sector_mask);
    m_status[sidx] = status;
  }

  virtual unsigned long long get_last_access_time() {
    return m_line_last_access_time;
  }

  virtual void set_last_access_time(unsigned long long time,
                                    mem_access_sector_mask_t sector_mask) {
    unsigned sidx = get_sector_index(sector_mask);

    m_last_sector_access_time[sidx] = time;
    m_line_last_access_time = time;
  }

  virtual unsigned long long get_alloc_time() { return m_line_alloc_time; }

  virtual void set_ignore_on_fill(bool m_ignore,
                                  mem_access_sector_mask_t sector_mask) {
    unsigned sidx = get_sector_index(sector_mask);
    m_ignore_on_fill_status[sidx] = m_ignore;
  }

  virtual void set_modified_on_fill(bool m_modified,
                                    mem_access_sector_mask_t sector_mask) {
    unsigned sidx = get_sector_index(sector_mask);
    m_set_modified_on_fill[sidx] = m_modified;
  }

  virtual void set_m_readable(bool readable,
                              mem_access_sector_mask_t sector_mask) {
    unsigned sidx = get_sector_index(sector_mask);
    m_readable[sidx] = readable;
  }

  virtual bool is_readable(mem_access_sector_mask_t sector_mask) {
    unsigned sidx = get_sector_index(sector_mask);
    return m_readable[sidx];
  }

  virtual unsigned get_modified_size() {
    unsigned modified = 0;
    for (unsigned i = 0; i < SECTOR_CHUNCK_SIZE; ++i) {
      if (m_status[i] == MODIFIED) modified++;
    }
    return modified * SECTOR_SIZE;
  }

  virtual void print_status() {
    printf("m_block_addr is %llu, status = %u %u %u %u\n", m_block_addr,
           m_status[0], m_status[1], m_status[2], m_status[3]);
  }

 private:
  unsigned m_sector_alloc_time[SECTOR_CHUNCK_SIZE];
  unsigned m_last_sector_access_time[SECTOR_CHUNCK_SIZE];
  unsigned m_sector_fill_time[SECTOR_CHUNCK_SIZE];
  unsigned m_line_alloc_time;
  unsigned m_line_last_access_time;
  unsigned m_line_fill_time;
  cache_block_state m_status[SECTOR_CHUNCK_SIZE];
  bool m_ignore_on_fill_status[SECTOR_CHUNCK_SIZE];
  bool m_set_modified_on_fill[SECTOR_CHUNCK_SIZE];
  bool m_readable[SECTOR_CHUNCK_SIZE];

  unsigned get_sector_index(mem_access_sector_mask_t sector_mask) {
    assert(sector_mask.count() == 1);
    for (unsigned i = 0; i < SECTOR_CHUNCK_SIZE; ++i) {
      if (sector_mask.to_ulong() & (1 << i)) return i;
    }
  }
};
struct elastic_cache_block : public cache_block_t {
  elastic_cache_block() { init(); }

  void init() {
    for (unsigned i = 0; i < 4; ++i) {  //Elastic: Come back to see if it is configurable
      m_word_alloc_time[i] = 0;
      m_word_fill_time[i] = 0;
      m_word_last_access_time[i] = 0;
      m_word_status[i] = INVALID;
      m_ignore_word_on_fill_status[i] = false;
      m_set_word_modified_on_fill[i] = false;
      m_word_readable[i] = true;
    }
    m_line_alloc_time = 0;
    m_line_last_access_time = 0;
    m_line_fill_time = 0;
    m_ignore_line_on_fill_status = false;
    m_set_line_modified_on_fill = false;
    m_line_readable = true;
  }

  virtual void allocate(new_addr_type tag, new_addr_type addr,
                        unsigned time, mem_access_sector_mask_t sector_mask) {
    allocate_line(tag, addr, time, sector_mask);
  }

  void allocate_line(new_addr_type tag, new_addr_type addr, unsigned time,
                     mem_access_sector_mask_t sector_mask) {
    init();
    m_common_tag = (tag>>26);  //Elastic:extract upper 6 bits for common tag
    m_addr = addr;      //Added for set_word_info use
    
    //Elastic: we allocate an entire line. So we direct map it to a bank
    unsigned bidx = get_bank_index(sector_mask);
    new_addr_type byte_index_upper_2_bits = (addr>>5)&0x3;
    new_addr_type chunk_tag_14_bits = (addr>>12)&0x3fff;
    m_chunk_tag[bidx]= ( (chunk_tag_14_bits<<2) | byte_index_upper_2_bits); //Elastic:Fill chunk in location

  //m_chunk_tag[bidx]= 
    // set sector stats
    m_word_alloc_time[bidx] = time;
    m_word_last_access_time[bidx] = time;
    m_word_fill_time[bidx] = 0;
    m_word_status[bidx] = RESERVED;
    m_ignore_word_on_fill_status[bidx] = false;
    m_set_word_modified_on_fill[bidx] = false;

    // set line stats
    m_line_alloc_time = time;  // only set this for the first allocated sector
    m_line_last_access_time = time;
    m_line_fill_time = 0;
    m_ignore_line_on_fill_status = false;
    m_set_line_modified_on_fill = false;
  }
   //Elastic: For the following func, we pass bank id also. Come back
  void allocate_word(new_addr_type addr, unsigned time,unsigned bidx) {
    // allocate invalid sector of this allocated valid line
    //assert(is_valid_line());//Elastic: commenting for debug
    assert(bidx<4 && bidx>=0);
    //unsigned sidx = get_sector_index(sector_mask);
    new_addr_type byte_index_upper_2_bits = (addr>>5)&0x3;
    new_addr_type chunk_tag_14_bits = (addr>>12)&0x3fff;
    m_chunk_tag[bidx]= ( (chunk_tag_14_bits<<2) | byte_index_upper_2_bits); //Elastic:Fill chunk in location 
    // set sector stats
    m_word_alloc_time[bidx] = time;
    m_word_last_access_time[bidx] = time;
    m_word_fill_time[bidx] = 0;
    if (m_word_status[bidx] == MODIFIED)  // this should be the case only for
                                     // fetch-on-write policy //TO DO
      m_set_word_modified_on_fill[bidx] = true;
    else
      m_set_word_modified_on_fill[bidx] = false;

    m_word_status[bidx] = RESERVED;
    m_ignore_word_on_fill_status[bidx] = false;
    // m_set_modified_on_fill[sidx] = false;
    m_word_readable[bidx] = true;

    // set line stats
    m_line_last_access_time = time;
    m_line_fill_time = 0;
  } 
  //to avoid linker error
  virtual void fill(unsigned time, mem_access_sector_mask_t sector_mask){
   }
  //Elastic: Come back to modify
  virtual void elastic_fill(unsigned time, unsigned bidx){
    //unsigned sidx = get_sector_index(sector_mask);

    //	if(!m_ignore_on_fill_status[sidx])
    //	         assert( m_status[sidx] == RESERVED );
    
    assert(bidx<4 && bidx>=0);
    m_word_status[bidx] = m_set_word_modified_on_fill[bidx] ? MODIFIED : VALID;
    //m_line_status       = m_set_line_modified_on_fill ? MODIFIED : VALID; //Mightn need to change
    m_line_status = m_word_status[bidx];
    m_word_fill_time[bidx] = time;
    m_line_fill_time = time;
  }
  virtual bool is_invalid_line() {
    // all the words should be invalid for the line
   // for (unsigned i = 0; i < SECTOR_CHUNCK_SIZE; ++i) {
    for (unsigned i = 0; i < 4; ++i) { //Elastic:Come back to configure later
      if (m_word_status[i] != INVALID) return false;
    }
    return true;
  }
  virtual bool is_invalid_word(unsigned bidx) {
      assert(bidx<4 && bidx>=0);
      if (m_word_status[bidx] != INVALID) return false;
    return true;
  }
  virtual bool is_valid_word(unsigned bidx) { 
      assert(bidx<4 && bidx>=0);
      return !(is_invalid_word(bidx)); 
  }
  virtual bool is_valid_line() { return !(is_invalid_line()); }
  virtual bool is_reserved_line() {
    // if all the words are reserved, then the line is reserved
    //for (unsigned i = 0; i < SECTOR_CHUNCK_SIZE; ++i)
    for (unsigned i = 0; i < 4; ++i) { //Elastic:Come back to configure later
      if (m_word_status[i] != HIT_WORD_RESERVED) return false;
    }
    return true;
  }
  virtual bool is_reserved_word(unsigned bidx) {
    // if all the words are reserved, then the line is reserved
    assert(bidx<4 && bidx>=0);
    if (m_word_status[bidx] == HIT_WORD_RESERVED) return true;
    return false;
  }
  virtual bool is_modified_line() {
    // if any of the word is modified, then the line is modified
    //for (unsigned i = 0; i < SECTOR_CHUNCK_SIZE; ++i) 
    for (unsigned i = 0; i < 4; ++i) { //Elastic:Come back to configure later
      if (m_word_status[i] == MODIFIED) return true;
    }
    return false;
  }
  virtual bool is_modified_word(unsigned bidx) {
    assert(bidx<4 && bidx>=0);
    if (m_word_status[bidx] == MODIFIED) return true;
    return false;
  }

  virtual enum cache_block_state get_status(mem_access_sector_mask_t sector_mask){}
  virtual enum cache_block_state get_word_status(unsigned bidx) {
    assert(bidx<4 && bidx>=0);
    return m_word_status[bidx];
  }

  virtual void set_status(enum cache_block_state status,mem_access_sector_mask_t sector_mask) {}
  virtual void set_word_status(enum cache_block_state status,
		unsigned bidx) {
    assert(bidx<4 && bidx>=0);
    m_word_status[bidx] = status ;
    if(m_word_status[bidx] == MODIFIED)
    	m_line_status = status ;
    if(m_word_status[bidx] == VALID)
        m_line_status = status;
  }


  virtual unsigned long long get_last_access_time() {
    return m_line_last_access_time;
  }
  virtual unsigned long long get_word_last_access_time(unsigned bidx) {
    assert(bidx<4 && bidx>=0);
    return m_word_last_access_time[bidx];
  }

  virtual void set_last_access_time(unsigned long long time,
                                    mem_access_sector_mask_t sector_mask) {
    unsigned bidx = get_bank_index(sector_mask);
    assert(bidx<4 && bidx>=0);

    m_word_last_access_time[bidx] = time;
    m_line_last_access_time = time;
  }
  virtual void set_word_last_access_time(unsigned long long time,unsigned bidx) {

    assert(bidx<4 && bidx>=0);
    m_word_last_access_time[bidx] = time;
    m_line_last_access_time = time;
  }

  virtual unsigned long long get_alloc_time() { return m_line_alloc_time; }
  virtual unsigned long long get_word_alloc_time(unsigned bidx) { 
    assert(bidx<4 && bidx>=0);
    return m_word_alloc_time[bidx]; 
   }

  virtual void set_ignore_on_fill(bool m_ignore,
                                  mem_access_sector_mask_t sector_mask) {
    unsigned bidx = get_bank_index(sector_mask);
    assert(bidx<4 && bidx>=0);
    m_ignore_word_on_fill_status[bidx] = m_ignore;
    m_ignore_line_on_fill_status = m_ignore;
  }
  virtual void set_word_ignore_on_fill(bool m_ignore,unsigned bidx){
    assert(bidx<4 && bidx>=0);
    m_ignore_word_on_fill_status[bidx] = m_ignore;
  }

  virtual void set_modified_on_fill(bool m_modified,
                                    mem_access_sector_mask_t sector_mask) {
    unsigned bidx = get_bank_index(sector_mask);
    assert(bidx<4 && bidx>=0);
    m_set_word_modified_on_fill[bidx] = m_modified;
    m_set_line_modified_on_fill = m_modified;
  }

  virtual void set_m_readable(bool readable,
                              mem_access_sector_mask_t sector_mask) {
    unsigned bidx = get_bank_index(sector_mask);
    assert(bidx<4 && bidx>=0);
    m_word_readable[bidx] = readable;
    m_line_readable = readable;
  }

  virtual void set_m_word_readable(bool readable,unsigned bidx){
    assert(bidx<4 && bidx>=0);
    m_word_readable[bidx] = readable;
  }
  //Elastic: May not be used? Come back 
  virtual bool is_readable(mem_access_sector_mask_t sector_mask) {
    unsigned bidx = get_bank_index(sector_mask);
    assert(bidx<4 && bidx>=0);
    return m_word_readable[bidx];
  }
  virtual bool is_word_readable(unsigned bidx){
    assert(bidx<4 && bidx>=0);
    return m_word_readable[bidx];
  }

  virtual unsigned get_modified_size() {
    unsigned modified = 0;
    //for (unsigned i = 0; i < SECTOR_CHUNCK_SIZE; ++i) 
    for (unsigned i = 0; i < 4; ++i) {  //Elastic: Come back to Configure later
      if (m_word_status[i] == MODIFIED) modified++;
    }
    //return modified * SECTOR_SIZE;
    return modified * 32;
  }
  //Elastic: Come back to print more(for eg: set_index) for debug purpose
  virtual void print_status() {
    printf("m_commom_tag is %llu, m_chunk_tag = %u %u %u %u ,word_status = %u %u %u %u ,\n", (unsigned)m_common_tag,(unsigned)m_chunk_tag[0],(unsigned)m_chunk_tag[1],(unsigned)m_chunk_tag[2],(unsigned)m_chunk_tag[3],
           m_word_status[0], m_word_status[1], m_word_status[2], m_word_status[3]);
  }

 private:
  //Elastic:Come back to configure later
  //unsigned m_sector_alloc_time[SECTOR_CHUNCK_SIZE];
  //unsigned m_last_sector_access_time[SECTOR_CHUNCK_SIZE];
  //unsigned m_sector_fill_time[SECTOR_CHUNCK_SIZE];
  //unsigned m_line_alloc_time;
  //unsigned m_line_last_access_time;
  //unsigned m_line_fill_time;
  //cache_block_state m_status[SECTOR_CHUNCK_SIZE];
  //bool m_ignore_on_fill_status[SECTOR_CHUNCK_SIZE];
  //bool m_set_modified_on_fill[SECTOR_CHUNCK_SIZE];
  //bool m_readable[SECTOR_CHUNCK_SIZE];
  unsigned m_word_alloc_time[4];
  unsigned m_word_last_access_time[4];
  unsigned m_word_fill_time[4];
  unsigned m_line_alloc_time;
  unsigned m_line_last_access_time;
  unsigned m_line_fill_time;
  cache_block_state m_word_status[4];
  bool m_ignore_word_on_fill_status[4];
  bool m_set_word_modified_on_fill[4];
  bool m_word_readable[4];
  bool m_ignore_line_on_fill_status;
  bool m_set_line_modified_on_fill;
  bool m_line_readable;
  cache_block_state m_line_status;

  unsigned get_bank_index(mem_access_sector_mask_t sector_mask) {
    assert(sector_mask.count() == 1);
    //for (unsigned i = 0; i < SECTOR_CHUNCK_SIZE; ++i)
    for (unsigned i = 0; i < 4; ++i) {//Elastic: Come back later to configure
      if (sector_mask.to_ulong() & (1 << i)) return i;
    }
  }
};
enum replacement_policy_t { LRU, FIFO };

enum write_policy_t {
  READ_ONLY,
  WRITE_BACK,
  WRITE_THROUGH,
  WRITE_EVICT,
  LOCAL_WB_GLOBAL_WT
};

enum allocation_policy_t { ON_MISS, ON_FILL, STREAMING };

enum write_allocate_policy_t {
  NO_WRITE_ALLOCATE,
  WRITE_ALLOCATE,
  FETCH_ON_WRITE,
  LAZY_FETCH_ON_READ
};

enum mshr_config_t {
  TEX_FIFO,         // Tex cache
  ASSOC,            // normal cache
  SECTOR_TEX_FIFO,  // Tex cache sends requests to high-level sector cache
  SECTOR_ASSOC,      // normal cache sends requests to high-level sector cache
  ELASTIC_ASSOC
};

enum set_index_function {
  LINEAR_SET_FUNCTION = 0,
  BITWISE_XORING_FUNCTION,
  HASH_IPOLY_FUNCTION,
  FERMI_HASH_SET_FUNCTION,
  CUSTOM_SET_FUNCTION
};

enum cache_type { NORMAL = 0, SECTOR,ELASTIC };

#define MAX_WARP_PER_SHADER 64
#define INCT_TOTAL_BUFFER 64
#define L2_TOTAL 64
#define MAX_WARP_PER_SHADER 64
#define MAX_WARP_PER_SHADER 64

class cache_config {
 public:
  cache_config() {
    m_valid = false;
    m_disabled = false;
    m_config_string = NULL;  // set by option parser
    m_config_stringPrefL1 = NULL;
    m_config_stringPrefShared = NULL;
    m_data_port_width = 0;
    m_set_index_function = LINEAR_SET_FUNCTION;
    m_is_streaming = false;
    m_config_stringPrefElasticL1 = NULL; //Elastic: Added new config
    m_config_stringPrefElasticShared = NULL; //Elastic: Added new config

  }
  void init(char *config, FuncCache status) {
    cache_status = status;
    assert(config);
    char ct, rp, wp, ap, mshr_type, wap, sif;

    int ntok =
        sscanf(config, "%c:%u:%u:%u,%c:%c:%c:%c:%c,%c:%u:%u,%u:%u,%u", &ct,
               &m_nset, &m_line_sz, &m_assoc, &rp, &wp, &ap, &wap, &sif,
               &mshr_type, &m_mshr_entries, &m_mshr_max_merge,
               &m_miss_queue_size, &m_result_fifo_entries, &m_data_port_width);

    if (ntok < 12) {
      if (!strcmp(config, "none")) {
        m_disabled = true;
        return;
      }
      exit_parse_error();
    }

    switch (ct) {
      case 'N':
        m_cache_type = NORMAL;
        break;
      case 'S':
        m_cache_type = SECTOR;
        break;
      case 'E':
        m_cache_type = ELASTIC;    //Elastic: Added for elastic cache
        break; 
      default:
        exit_parse_error();
    }
    switch (rp) {
      case 'L':
        m_replacement_policy = LRU;
        break;
      case 'F':
        m_replacement_policy = FIFO;
        break;
      default:
        exit_parse_error();
    }
    switch (rp) {
      case 'L':
        m_replacement_policy = LRU;
        break;
      case 'F':
        m_replacement_policy = FIFO;
        break;
      default:
        exit_parse_error();
    }
    switch (wp) {
      case 'R':
        m_write_policy = READ_ONLY;
        break;
      case 'B':
        m_write_policy = WRITE_BACK;
        break;
      case 'T':
        m_write_policy = WRITE_THROUGH;
        break;
      case 'E':
        m_write_policy = WRITE_EVICT;
        break;
      case 'L':
        m_write_policy = LOCAL_WB_GLOBAL_WT;
        break;
      default:
        exit_parse_error();
    }
    switch (ap) {
      case 'm':
        m_alloc_policy = ON_MISS;
        break;
      case 'f':
        m_alloc_policy = ON_FILL;
        break;
      case 's':
        m_alloc_policy = STREAMING;
        break;
      default:
        exit_parse_error();
    }
    if (m_alloc_policy == STREAMING) {
      // For streaming cache, we set the alloc policy to be on-fill to remove
      // all line_alloc_fail stalls we set the MSHRs to be equal to max
      // allocated cache lines. This is possible by moving TAG to be shared
      // between cache line and MSHR enrty (i.e. for each cache line, there is
      // an MSHR rntey associated with it) This is the easiest think we can
      // think about to model (mimic) L1 streaming cache in Pascal and Volta
      // Based on our microbenchmakrs, MSHRs entries have been increasing
      // substantially in Pascal and Volta For more information about streaming
      // cache, see:
      // http://on-demand.gputechconf.com/gtc/2017/presentation/s7798-luke-durant-inside-volta.pdf
      // https://ieeexplore.ieee.org/document/8344474/
      m_is_streaming = true;
      m_alloc_policy = ON_FILL;
      m_mshr_entries = m_nset * m_assoc * MAX_DEFAULT_CACHE_SIZE_MULTIBLIER;
      if (m_cache_type == SECTOR) m_mshr_entries *= SECTOR_CHUNCK_SIZE;
      else if (m_cache_type == ELASTIC) m_mshr_entries *= 4;  //Elastic: Come back to configure later
      m_mshr_max_merge = MAX_WARP_PER_SM;
     
    }
    switch (mshr_type) {
      case 'F':
        m_mshr_type = TEX_FIFO;
        assert(ntok == 14);
        break;
      case 'T':
        m_mshr_type = SECTOR_TEX_FIFO;
        assert(ntok == 14);
        break;
      case 'A':
        m_mshr_type = ASSOC;
        break;
      case 'S':
        m_mshr_type = SECTOR_ASSOC;
        break;
      case 'E':
        m_mshr_type = ELASTIC_ASSOC;
        break;
      default:
        exit_parse_error();
    }
    m_line_sz_log2 = LOGB2(m_line_sz);
    m_nset_log2 = LOGB2(m_nset);
    m_valid = true;
     switch(m_cache_type){
      case NORMAL:
        m_atom_sz = m_line_sz;
        break;
      case SECTOR:
        m_atom_sz = SECTOR_SIZE;
        break;
      case ELASTIC:
        m_atom_sz = 32;   //Elastic:Come back to configure later
        break;
      }

    m_sector_sz_log2 = LOGB2(SECTOR_SIZE);
    original_m_assoc = m_assoc;
   m_word_sz_log2 = LOGB2(32);
    // For more details about difference between FETCH_ON_WRITE and WRITE
    // VALIDAE policies Read: Jouppi, Norman P. "Cache write policies and
    // performance". ISCA 93. WRITE_ALLOCATE is the old write policy in
    // GPGPU-sim 3.x, that send WRITE and READ for every write request
    switch (wap) {
      case 'N':
        m_write_alloc_policy = NO_WRITE_ALLOCATE;
        break;
      case 'W':
        m_write_alloc_policy = WRITE_ALLOCATE;
        break;
      case 'F':
        m_write_alloc_policy = FETCH_ON_WRITE;
        break;
      case 'L':
        m_write_alloc_policy = LAZY_FETCH_ON_READ;
        break;
      default:
        exit_parse_error();
    }

    // detect invalid configuration
    if (m_alloc_policy == ON_FILL and m_write_policy == WRITE_BACK) {
      // A writeback cache with allocate-on-fill policy will inevitably lead to
      // deadlock: The deadlock happens when an incoming cache-fill evicts a
      // dirty line, generating a writeback request.  If the memory subsystem is
      // congested, the interconnection network may not have sufficient buffer
      // for the writeback request.  This stalls the incoming cache-fill.  The
      // stall may propagate through the memory subsystem back to the output
      // port of the same core, creating a deadlock where the wrtieback request
      // and the incoming cache-fill are stalling each other.
      assert(0 &&
             "Invalid cache configuration: Writeback cache cannot allocate new "
             "line on fill. ");
    }

    if ((m_write_alloc_policy == FETCH_ON_WRITE ||
         m_write_alloc_policy == LAZY_FETCH_ON_READ) &&
        m_alloc_policy == ON_FILL) {
      assert(
          0 &&
          "Invalid cache configuration: FETCH_ON_WRITE and LAZY_FETCH_ON_READ "
          "cannot work properly with ON_FILL policy. Cache must be ON_MISS. ");
    }
    if (m_cache_type == SECTOR) {
      assert(m_line_sz / SECTOR_SIZE == SECTOR_CHUNCK_SIZE &&
             m_line_sz % SECTOR_SIZE == 0);
    }
     else if (m_cache_type == ELASTIC) {
      assert(m_line_sz / 32 == 4 &&
             m_line_sz % 32 == 0);
    }

    // default: port to data array width and granularity = line size
    if (m_data_port_width == 0) {
      m_data_port_width = m_line_sz;
    }
    assert(m_line_sz % m_data_port_width == 0);

    switch (sif) {
      case 'H':
        m_set_index_function = FERMI_HASH_SET_FUNCTION;
        break;
      case 'P':
        m_set_index_function = HASH_IPOLY_FUNCTION;
        break;
      case 'C':
        m_set_index_function = CUSTOM_SET_FUNCTION;
        break;
      case 'L':
        m_set_index_function = LINEAR_SET_FUNCTION;
        break;
      default:
        exit_parse_error();
    }
  }
  bool disabled() const { return m_disabled; }
  unsigned get_line_sz() const {
    assert(m_valid);
    return m_line_sz;
  }
  unsigned get_atom_sz() const {
    assert(m_valid);
    return m_atom_sz;
  }
  unsigned get_num_lines() const {
    assert(m_valid);
    return m_nset * m_assoc;
  }
  unsigned get_max_num_lines() const {
    assert(m_valid);
    return MAX_DEFAULT_CACHE_SIZE_MULTIBLIER * m_nset * original_m_assoc;
  }
  unsigned get_max_assoc() const {
    assert(m_valid);
    return MAX_DEFAULT_CACHE_SIZE_MULTIBLIER * original_m_assoc;
  }
  void print(FILE *fp) const {
    fprintf(fp, "Size = %d B (%d Set x %d-way x %d byte line)\n",
            m_line_sz * m_nset * m_assoc, m_nset, m_assoc, m_line_sz);
  }

  virtual unsigned set_index(new_addr_type addr) const {
    if (m_set_index_function != LINEAR_SET_FUNCTION) {
      printf(
          "\nGPGPU-Sim cache configuration error: Hashing or "
          "custom set index function selected in configuration "
          "file for a cache that has not overloaded the set_index "
          "function\n");
      abort();
    }
    return (addr >> m_line_sz_log2) & (m_nset - 1);
  }
  //Elastic: Added common_tag and chunk_tag functions
  new_addr_type common_tag(new_addr_type addr) const {
    assert(m_cache_type==ELASTIC);
    return (addr >>26);
  }
  //Elastic: Come back later to configure
  new_addr_type chunk_tag(new_addr_type addr) const {
  	assert(m_cache_type==ELASTIC);
    new_addr_type byte_index_upper_2_bits = (addr>>5)&0x3;
    new_addr_type chunk_tag_14_bits = (addr>>12)&0x3fff;
    return ( (chunk_tag_14_bits<<2) | byte_index_upper_2_bits);
  }

  new_addr_type tag(new_addr_type addr) const {
    // For generality, the tag includes both index and tag. This allows for more
    // complex set index calculations that can result in different indexes
    // mapping to the same set, thus the full tag + index is required to check
    // for hit/miss. Tag is now identical to the block address.

    // return addr >> (m_line_sz_log2+m_nset_log2);
    return addr & ~(new_addr_type)(m_line_sz - 1);
  }
  new_addr_type block_addr(new_addr_type addr) const {
    return addr & ~(new_addr_type)(m_line_sz - 1);
  }
  new_addr_type mshr_addr(new_addr_type addr) const {
    return addr & ~(new_addr_type)(m_atom_sz - 1);
  }
  enum mshr_config_t get_mshr_type() const { return m_mshr_type; }
  enum cache_type get_cache_type() const { return m_cache_type; } //Elastic:Added for access from shader.cc

  void set_assoc(unsigned n) {
    // set new assoc. L1 cache dynamically resized in Volta
    m_assoc = n;
  }
  unsigned get_nset() const {
    assert(m_valid);
    return m_nset;
  }
  unsigned get_total_size_inKB() const {
    assert(m_valid);
    return (m_assoc * m_nset * m_line_sz) / 1024;
  }
  bool is_streaming() { return m_is_streaming; }
  FuncCache get_cache_status() { return cache_status; }
  char *m_config_string;
  char *m_config_stringPrefL1;
  char *m_config_stringPrefShared;
  char *m_config_stringPrefElasticL1;
  char *m_config_stringPrefElasticShared; 
  FuncCache cache_status;

 protected:
  void exit_parse_error() {
    printf("GPGPU-Sim uArch: cache configuration parsing error (%s)\n",
           m_config_string);
    abort();
  }

  bool m_valid;
  bool m_disabled;
  unsigned m_line_sz;
  unsigned m_line_sz_log2;
  unsigned m_nset;
  unsigned m_nset_log2;
  unsigned m_assoc;
  unsigned m_atom_sz;
  unsigned m_sector_sz_log2;
  unsigned original_m_assoc;
  bool m_is_streaming;

  enum replacement_policy_t m_replacement_policy;  // 'L' = LRU, 'F' = FIFO
  enum write_policy_t
      m_write_policy;  // 'T' = write through, 'B' = write back, 'R' = read only
  enum allocation_policy_t
      m_alloc_policy;  // 'm' = allocate on miss, 'f' = allocate on fill
  enum mshr_config_t m_mshr_type;
  enum cache_type m_cache_type;

  write_allocate_policy_t
      m_write_alloc_policy;  // 'W' = Write allocate, 'N' = No write allocate

  union {
    unsigned m_mshr_entries;
    unsigned m_fragment_fifo_entries;
  };
  union {
    unsigned m_mshr_max_merge;
    unsigned m_request_fifo_entries;
  };
  union {
    unsigned m_miss_queue_size;
    unsigned m_rob_entries;
  };
  unsigned m_result_fifo_entries;
  unsigned m_data_port_width;  //< number of byte the cache can access per cycle
  enum set_index_function
      m_set_index_function;  // Hash, linear, or custom set index function
unsigned m_word_sz_log2;
  friend class tag_array;
  friend class elastic_tag_array;
  //friend class l1_tag_array;
  //friend class l2_tag_array;
  friend class baseline_cache;
  friend class read_only_cache;
  friend class tex_cache;
  friend class data_cache;
  friend class l1_cache;
  friend class l1_elastic_cache;
  friend class l2_cache;
  friend class l2_elastic_cache;
  friend class memory_sub_partition;
};

class l1d_cache_config : public cache_config {
 public:
  l1d_cache_config() : cache_config() {}
  virtual unsigned set_index(new_addr_type addr) const;
  unsigned set_bank(new_addr_type addr) const;
  unsigned l1_latency;
  unsigned l1_banks;
};

class l2_cache_config : public cache_config {
 public:
  l2_cache_config() : cache_config() {}
  void init(linear_to_raw_address_translation *address_mapping);
  virtual unsigned set_index(new_addr_type addr) const;

 private:
  linear_to_raw_address_translation *m_address_mapping;
};

class tag_array {
 public:
  // Use this constructor
  tag_array(cache_config &config, int core_id, int type_id);
  ~tag_array();

  enum cache_request_status probe(new_addr_type addr, unsigned &idx,
                                  mem_fetch *mf, bool probe_mode = false) const;
  enum cache_request_status probe(new_addr_type addr, unsigned &idx,
                                  mem_access_sector_mask_t mask,
                                  bool probe_mode = false,
                                  mem_fetch *mf = NULL) const;
  enum cache_request_status access(new_addr_type addr, unsigned time,
                                   unsigned &idx, mem_fetch *mf);
  enum cache_request_status access(new_addr_type addr, unsigned time,
                                   unsigned &idx, bool &wb,
                                   evicted_block_info &evicted, mem_fetch *mf);

  void fill(new_addr_type addr, unsigned time, mem_fetch *mf);
  void fill(unsigned idx, unsigned time, mem_fetch *mf);
  void fill(new_addr_type addr, unsigned time, mem_access_sector_mask_t mask);

  unsigned size() const { return m_config.get_num_lines(); }
  cache_block_t *get_block(unsigned idx) { return m_lines[idx]; }

  void flush();       // flush all written entries
  void invalidate();  // invalidate all entries
  void new_window();

  void print(FILE *stream, unsigned &total_access,
             unsigned &total_misses) const;
  float windowed_miss_rate() const;
  void get_stats(unsigned &total_access, unsigned &total_misses,
                 unsigned &total_hit_res, unsigned &total_res_fail) const;

  void update_cache_parameters(cache_config &config);
  void add_pending_line(mem_fetch *mf);
  void remove_pending_line(mem_fetch *mf);

 protected:
  // This constructor is intended for use only from derived classes that wish to
  // avoid unnecessary memory allocation that takes place in the
  // other tag_array constructor
  tag_array(cache_config &config, int core_id, int type_id,
            cache_block_t **new_lines);
  void init(int core_id, int type_id);

 protected:
  cache_config &m_config;

  cache_block_t **m_lines; /* nbanks x nset x assoc lines in total */

  unsigned m_access;
  unsigned m_miss;
  unsigned m_pending_hit;  // number of cache miss that hit a line that is
                           // allocated but not filled
  unsigned m_res_fail;
  unsigned m_sector_miss;

  // performance counters for calculating the amount of misses within a time
  // window
  unsigned m_prev_snapshot_access;
  unsigned m_prev_snapshot_miss;
  unsigned m_prev_snapshot_pending_hit;

  int m_core_id;  // which shader core is using this
  int m_type_id;  // what kind of cache is this (normal, texture, constant)

  bool is_used;  // a flag if the whole cache has ever been accessed before

  typedef tr1_hash_map<new_addr_type, unsigned> line_table;
  line_table pending_lines;
};
//Elastic: Following class is added for l1_tag_array access
class elastic_tag_array {
 public:
  // Use this constructor
  elastic_tag_array(cache_config &config, int core_id, int type_id);
  ~elastic_tag_array();

  //Elastic: Added &widx to probe,access and one of the fill functions
  enum cache_request_status probe(new_addr_type addr, unsigned &idx, unsigned &widx,
                                  mem_fetch *mf, bool probe_mode = false) const;
  enum cache_request_status probe(new_addr_type addr, unsigned &idx,unsigned &widx,
                                  mem_access_sector_mask_t mask,
                                  bool probe_mode = false,
                                  mem_fetch *mf = NULL) const;
  enum cache_request_status access(new_addr_type addr, unsigned time,
                                   unsigned &idx,unsigned &widx, mem_fetch *mf);
  enum cache_request_status access(new_addr_type addr, unsigned time,
                                   unsigned &idx, unsigned &widx, bool &wb,
                                   evicted_block_info &evicted_line ,evicted_word_info &evicted_word,mem_fetch *mf);

  void fill(new_addr_type addr, unsigned time, mem_fetch *mf);
  void fill(unsigned idx,unsigned widx, unsigned time, mem_fetch *mf);
  void fill(new_addr_type addr, unsigned time, mem_access_sector_mask_t mask);

  unsigned size() const { 
    assert(m_config.m_cache_type==ELASTIC);
  	return m_config.get_num_lines(); 
  }
  //Elastic: Modified get_block to used widx also
  cache_block_t *get_block(unsigned idx,unsigned widx) { 
    assert(m_config.m_cache_type==ELASTIC);
    assert(widx>=0 && widx<4);
  	return m_lines[idx]; 
  }

  void flush();       // flush all written entries
  void invalidate();  // invalidate all entries
  void new_window();

  void print(FILE *stream, unsigned &total_access,
             unsigned &total_misses) const;
  float windowed_miss_rate() const;
  void get_stats(unsigned &total_access, unsigned &total_misses,
                 unsigned &total_hit_res, unsigned &total_res_fail) const;

  void update_cache_parameters(cache_config &config);
  void add_pending_line(mem_fetch *mf);
  void remove_pending_line(mem_fetch *mf);

 protected:
  // This constructor is intended for use only from derived classes that wish to
  // avoid unnecessary memory allocation that takes place in the
  // other tag_array constructor
  elastic_tag_array(cache_config &config, int core_id, int type_id,
            cache_block_t **new_lines);
  void init(int core_id, int type_id);

 protected:
  cache_config &m_config;

  cache_block_t **m_lines; /* nbanks x nset x assoc lines in total */

  unsigned m_access;
  unsigned m_miss;
  unsigned m_pending_hit;  // Elastic:number of cache miss that hit a line or a word that is
                           // allocated but not filled
  unsigned m_res_fail;
  unsigned m_sector_miss;//Elastic:Not used

  // performance counters for calculating the amount of misses within a time
  // window
  unsigned m_prev_snapshot_access;
  unsigned m_prev_snapshot_miss;
  unsigned m_prev_snapshot_pending_hit;

  int m_core_id;  // which shader core is using this
  int m_type_id;  // what kind of cache is this (normal, texture, constant)

  bool is_used;  // a flag if the whole cache has ever been accessed before

  typedef tr1_hash_map<new_addr_type, unsigned> line_table;
  line_table pending_lines;
  //Elastic: Added for word miss
  unsigned m_word_miss;
};

/*class l2_tag_array {
 public:
  // Use this constructor
  l2_tag_array(cache_config &config, int core_id, int type_id);
  ~l2_tag_array();

  //Elastic: Added &widx to probe,access and one of the fill functions
  enum cache_request_status probe(new_addr_type addr, unsigned &idx, unsigned &widx,
                                  mem_fetch *mf, bool probe_mode = false) const;
  enum cache_request_status probe(new_addr_type addr, unsigned &idx,unsigned &widx,
                                  mem_access_sector_mask_t mask,
                                  bool probe_mode = false,
                                  mem_fetch *mf = NULL) const;
  enum cache_request_status access(new_addr_type addr, unsigned time,
                                   unsigned &idx,unsigned &widx, mem_fetch *mf);
  enum cache_request_status access(new_addr_type addr, unsigned time,
                                   unsigned &idx, unsigned &widx, bool &wb,
                                   evicted_block_info &evicted_line ,evicted_word_info &evicted_word,mem_fetch *mf);

  void fill(new_addr_type addr, unsigned time, mem_fetch *mf);
  void fill(unsigned idx,unsigned widx, unsigned time, mem_fetch *mf);
  void fill(new_addr_type addr, unsigned time, mem_access_sector_mask_t mask);

  unsigned size() const { 
    assert(m_config.m_cache_type==ELASTIC);
  	return m_config.get_num_lines(); 
  }
  //Elastic: Modified get_block to used widx also
  cache_block_t *get_block(unsigned idx,unsigned widx) { 
    assert(m_config.m_cache_type==ELASTIC);
    assert(widx>=0 && widx<4);
  	return m_lines[idx]; 
  }

  void flush();       // flush all written entries
  void invalidate();  // invalidate all entries
  void new_window();

  void print(FILE *stream, unsigned &total_access,
             unsigned &total_misses) const;
  float windowed_miss_rate() const;
  void get_stats(unsigned &total_access, unsigned &total_misses,
                 unsigned &total_hit_res, unsigned &total_res_fail) const;

  void update_cache_parameters(cache_config &config);
  void add_pending_line(mem_fetch *mf);
  void remove_pending_line(mem_fetch *mf);

 protected:
  // This constructor is intended for use only from derived classes that wish to
  // avoid unnecessary memory allocation that takes place in the
  // other tag_array constructor
  l2_tag_array(cache_config &config, int core_id, int type_id,
            cache_block_t **new_lines);
  void init(int core_id, int type_id);

 protected:
  cache_config &m_config;

  cache_block_t **m_lines;*/ /* nbanks x nset x assoc lines in total */

/*  unsigned m_access;
  unsigned m_miss;
  unsigned m_pending_hit;  // Elastic:number of cache miss that hit a line or a word that is
                           // allocated but not filled
  unsigned m_res_fail;
  unsigned m_sector_miss;//Elastic:Not used

  // performance counters for calculating the amount of misses within a time
  // window
  unsigned m_prev_snapshot_access;
  unsigned m_prev_snapshot_miss;
  unsigned m_prev_snapshot_pending_hit;

  int m_core_id;  // which shader core is using this
  int m_type_id;  // what kind of cache is this (normal, texture, constant)

  bool is_used;  // a flag if the whole cache has ever been accessed before

  typedef tr1_hash_map<new_addr_type, unsigned> line_table;
  line_table pending_lines;
  //Elastic: Added for word miss
  unsigned m_word_miss;
};*/



class mshr_table {
 public:
  mshr_table(unsigned num_entries, unsigned max_merged)
      : m_num_entries(num_entries),
        m_max_merged(max_merged)
#if (tr1_hash_map_ismap == 0)
        ,
        m_data(2 * num_entries)
#endif
  {
  }

  /// Checks if there is a pending request to the lower memory level already
  bool probe(new_addr_type block_addr) const;
  /// Checks if there is space for tracking a new memory access
  bool full(new_addr_type block_addr) const;
  /// Add or merge this access
  void add(new_addr_type block_addr, mem_fetch *mf);
  /// Returns true if cannot accept new fill responses
  bool busy() const { return false; }
  /// Accept a new cache fill response: mark entry ready for processing
  void mark_ready(new_addr_type block_addr, bool &has_atomic);
  /// Returns true if ready accesses exist
  bool access_ready() const { return !m_current_response.empty(); }
  /// Returns next ready access
  mem_fetch *next_access();
  void display(FILE *fp) const;
  // Returns true if there is a pending read after write
  bool is_read_after_write_pending(new_addr_type block_addr);

  void check_mshr_parameters(unsigned num_entries, unsigned max_merged) {
    assert(m_num_entries == num_entries &&
           "Change of MSHR parameters between kernels is not allowed");
    assert(m_max_merged == max_merged &&
           "Change of MSHR parameters between kernels is not allowed");
  }

 private:
  // finite sized, fully associative table, with a finite maximum number of
  // merged requests
  const unsigned m_num_entries;
  const unsigned m_max_merged;

  struct mshr_entry {
    std::list<mem_fetch *> m_list;
    bool m_has_atomic;
    mshr_entry() : m_has_atomic(false) {}
  };
  typedef tr1_hash_map<new_addr_type, mshr_entry> table;
  typedef tr1_hash_map<new_addr_type, mshr_entry> line_table;
  table m_data;
  line_table pending_lines;

  // it may take several cycles to process the merged requests
  bool m_current_response_ready;
  std::list<new_addr_type> m_current_response;
};

/***************************************************************** Caches
 * *****************************************************************/
///
/// Simple struct to maintain cache accesses, misses, pending hits, and
/// reservation fails.
///
struct cache_sub_stats {
  unsigned long long accesses;
  unsigned long long misses;
  unsigned long long word_misses; //Elastic::added
  unsigned long long pending_hits;
  unsigned long long res_fails;

  unsigned long long port_available_cycles;
  unsigned long long data_port_busy_cycles;
  unsigned long long fill_port_busy_cycles;

  cache_sub_stats() { clear(); }
  void clear() {
    accesses = 0;
    misses = 0;
    word_misses=0;
    pending_hits = 0;
    res_fails = 0;
    port_available_cycles = 0;
    data_port_busy_cycles = 0;
    fill_port_busy_cycles = 0;
  }
  cache_sub_stats &operator+=(const cache_sub_stats &css) {
    ///
    /// Overloading += operator to easily accumulate stats
    ///
    accesses += css.accesses;
    misses += css.misses;
    word_misses += css.word_misses; //Elastic: Added a new stat
    pending_hits += css.pending_hits;
    res_fails += css.res_fails;
    port_available_cycles += css.port_available_cycles;
    data_port_busy_cycles += css.data_port_busy_cycles;
    fill_port_busy_cycles += css.fill_port_busy_cycles;
    return *this;
  }

  cache_sub_stats operator+(const cache_sub_stats &cs) {
    ///
    /// Overloading + operator to easily accumulate stats
    ///
    cache_sub_stats ret;
    ret.accesses = accesses + cs.accesses;
    ret.misses = misses + cs.misses;
    ret.word_misses = word_misses + cs.word_misses;//Elastic: Added a new stat
    ret.pending_hits = pending_hits + cs.pending_hits;
    ret.res_fails = res_fails + cs.res_fails;
    ret.port_available_cycles =
        port_available_cycles + cs.port_available_cycles;
    ret.data_port_busy_cycles =
        data_port_busy_cycles + cs.data_port_busy_cycles;
    ret.fill_port_busy_cycles =
        fill_port_busy_cycles + cs.fill_port_busy_cycles;
    return ret;
  }

  void print_port_stats(FILE *fout, const char *cache_name) const;
};

// Used for collecting AerialVision per-window statistics
struct cache_sub_stats_pw {
  unsigned accesses;
  unsigned write_misses;
  unsigned write_hits;
  unsigned write_pending_hits;
  unsigned write_res_fails;

  unsigned read_misses;
  unsigned read_hits;
  unsigned read_pending_hits;
  unsigned read_res_fails;

  cache_sub_stats_pw() { clear(); }
  void clear() {
    accesses = 0;
    write_misses = 0;
    write_hits = 0;
    write_pending_hits = 0;
    write_res_fails = 0;
    read_misses = 0;
    read_hits = 0;
    read_pending_hits = 0;
    read_res_fails = 0;
  }
  cache_sub_stats_pw &operator+=(const cache_sub_stats_pw &css) {
    ///
    /// Overloading += operator to easily accumulate stats
    ///
    accesses += css.accesses;
    write_misses += css.write_misses;
    read_misses += css.read_misses;
    write_pending_hits += css.write_pending_hits;
    read_pending_hits += css.read_pending_hits;
    write_res_fails += css.write_res_fails;
    read_res_fails += css.read_res_fails;
    return *this;
  }

  cache_sub_stats_pw operator+(const cache_sub_stats_pw &cs) {
    ///
    /// Overloading + operator to easily accumulate stats
    ///
    cache_sub_stats_pw ret;
    ret.accesses = accesses + cs.accesses;
    ret.write_misses = write_misses + cs.write_misses;
    ret.read_misses = read_misses + cs.read_misses;
    ret.write_pending_hits = write_pending_hits + cs.write_pending_hits;
    ret.read_pending_hits = read_pending_hits + cs.read_pending_hits;
    ret.write_res_fails = write_res_fails + cs.write_res_fails;
    ret.read_res_fails = read_res_fails + cs.read_res_fails;
    return ret;
  }
};

///
/// Cache_stats
/// Used to record statistics for each cache.
/// Maintains a record of every 'mem_access_type' and its resulting
/// 'cache_request_status' : [mem_access_type][cache_request_status]
///
class cache_stats {
 public:
  cache_stats();
  void clear();
  // Clear AerialVision cache stats after each window
  void clear_pw();
  void inc_stats(int access_type, int access_outcome);
  // Increment AerialVision cache stats
  void inc_stats_pw(int access_type, int access_outcome);
  void inc_fail_stats(int access_type, int fail_outcome);
  enum cache_request_status select_stats_status(
      enum cache_request_status probe, enum cache_request_status access) const;
  unsigned long long &operator()(int access_type, int access_outcome,
                                 bool fail_outcome);
  unsigned long long operator()(int access_type, int access_outcome,
                                bool fail_outcome) const;
  cache_stats operator+(const cache_stats &cs);
  cache_stats &operator+=(const cache_stats &cs);
  void print_stats(FILE *fout, const char *cache_name = "Cache_stats") const;
  void print_fail_stats(FILE *fout,
                        const char *cache_name = "Cache_fail_stats") const;

  unsigned long long get_stats(enum mem_access_type *access_type,
                               unsigned num_access_type,
                               enum cache_request_status *access_status,
                               unsigned num_access_status) const;
  void get_sub_stats(struct cache_sub_stats &css) const;

  // Get per-window cache stats for AerialVision
  void get_sub_stats_pw(struct cache_sub_stats_pw &css) const;

  void sample_cache_port_utility(bool data_port_busy, bool fill_port_busy);

 private:
  bool check_valid(int type, int status) const;
  bool check_fail_valid(int type, int fail) const;

  std::vector<std::vector<unsigned long long> > m_stats;
  // AerialVision cache stats (per-window)
  std::vector<std::vector<unsigned long long> > m_stats_pw;
  std::vector<std::vector<unsigned long long> > m_fail_stats;

  unsigned long long m_cache_port_available_cycles;
  unsigned long long m_cache_data_port_busy_cycles;
  unsigned long long m_cache_fill_port_busy_cycles;
};

class cache_t {
 public:
  virtual ~cache_t() {}
  virtual enum cache_request_status access_elastic(new_addr_type addr, mem_fetch *mf,
                                           unsigned time,
                                           std::list<cache_event> &events){}

  virtual enum cache_request_status access(new_addr_type addr, mem_fetch *mf,
                                           unsigned time,
                                           std::list<cache_event> &events){};//Elastic:Removed pure virtual

  // accessors for cache bandwidth availability
  virtual bool data_port_free() const = 0;
  virtual bool fill_port_free() const = 0;
};

bool was_write_sent(const std::list<cache_event> &events);
bool was_read_sent(const std::list<cache_event> &events);
bool was_writeallocate_sent(const std::list<cache_event> &events);
//Elastic: Equivalent elastic cache stand alone functions
bool was_write_sent_elastic(const std::list<cache_event> &events);
//bool was_read_sent_elastic(const std::list<cache_event> &events);
int was_read_sent_elastic(const std::list<cache_event> &events,bool word);
//bool was_writeallocate_sent_elastic(const std::list<cache_event> &events);
//int was_write_sent_elastic(const std::list<cache_event> &events, bool word);
  int was_writeallocate_sent_elastic(const std::list<cache_event> &events,bool word);
/// Baseline cache
/// Implements common functions for read_only_cache and data_cache
/// Each subclass implements its own 'access' function
class baseline_cache : public cache_t {
 public:
  //Elastic: extra arg at the last l1_true so that constructor can be overloaded
 //Elastic: Also binded last arg l1_true  
  baseline_cache(const char *name, cache_config &config, int core_id,
                 int type_id, mem_fetch_interface *memport,
                 enum mem_fetch_status status,bool elastic_true)
      : m_config(config),
        m_elastic_tag_array(new elastic_tag_array(config, core_id, type_id)), 
        m_mshrs(config.m_mshr_entries, config.m_mshr_max_merge),
        m_bandwidth_management(config),
        m_elastic_true(elastic_true) {
    assert(m_config.m_cache_type == ELASTIC);
    init(name, config, memport, status);
  }
  
  baseline_cache(const char *name, cache_config &config, int core_id,
                 int type_id, mem_fetch_interface *memport,
                 enum mem_fetch_status status)
      : m_config(config),
        m_tag_array(new tag_array(config, core_id, type_id)),
        m_mshrs(config.m_mshr_entries, config.m_mshr_max_merge),
        m_bandwidth_management(config),
	m_elastic_true(false)  {
    assert(m_config.m_cache_type != ELASTIC);
    init(name, config, memport, status);
  }

  void init(const char *name, const cache_config &config,
            mem_fetch_interface *memport, enum mem_fetch_status status) {
    m_name = name;
    assert(config.m_mshr_type == ASSOC || config.m_mshr_type == SECTOR_ASSOC || config.m_mshr_type ==ELASTIC_ASSOC);
    m_memport = memport;
    m_miss_queue_status = status;
  }

  virtual ~baseline_cache() { 
	if(m_elastic_true){
    	assert(m_config.m_cache_type == ELASTIC);
		delete m_elastic_tag_array;
		//delete m_l2_tag_array; 
    }
	else if(!m_elastic_true){
    	assert(m_config.m_cache_type != ELASTIC);
  		delete m_tag_array; 
    }
   
   
}

  void update_cache_parameters(cache_config &config) {
    m_config = config;
    if(m_config.m_cache_type != ELASTIC)
 		m_tag_array->update_cache_parameters(config);
    else if(m_config.m_cache_type == ELASTIC)
    	m_elastic_tag_array->update_cache_parameters(config);
    m_mshrs.check_mshr_parameters(config.m_mshr_entries,
                                  config.m_mshr_max_merge);
  }
//Elastic:Added
   virtual enum cache_request_status access_elastic(new_addr_type addr, mem_fetch *mf,
                                           unsigned time,
                                           std::list<cache_event> &events){}

  virtual enum cache_request_status access(new_addr_type addr, mem_fetch *mf,
                                           unsigned time,
                                           std::list<cache_event> &events){};

  /// Sends next request to lower level of memory
  void cycle();
  /// Interface for response from lower memory level (model bandwidth
  /// restictions in caller)
  void fill(mem_fetch *mf, unsigned time);
  /// Checks if mf is waiting to be filled by lower memory level
  bool waiting_for_fill(mem_fetch *mf);
  /// Are any (accepted) accesses that had to wait for memory now ready? (does
  /// not include accesses that "HIT")
  bool access_ready() const { return m_mshrs.access_ready(); }
  /// Pop next ready access (does not include accesses that "HIT")
  mem_fetch *next_access() { return m_mshrs.next_access(); }
  // flash invalidate all entries in cache
  void flush() { 
	if(m_config.m_cache_type != ELASTIC)
		m_tag_array->flush(); 
	else if(m_config.m_cache_type == ELASTIC)
    	m_elastic_tag_array->flush();
  }
  void invalidate() { 
	if(m_config.m_cache_type != ELASTIC)
  		m_tag_array->invalidate();
	else if(m_config.m_cache_type == ELASTIC)
    	m_elastic_tag_array->invalidate();
  }
  void print(FILE *fp, unsigned &accesses, unsigned &misses) const;
  void display_state(FILE *fp) const;

  // Stat collection
  const cache_stats &get_stats() const { return m_stats; }
  unsigned get_stats(enum mem_access_type *access_type,
                     unsigned num_access_type,
                     enum cache_request_status *access_status,
                     unsigned num_access_status) const {
    return m_stats.get_stats(access_type, num_access_type, access_status,
                             num_access_status);
  }
  void get_sub_stats(struct cache_sub_stats &css) const {
    m_stats.get_sub_stats(css);
  }
  // Clear per-window stats for AerialVision support
  void clear_pw() { m_stats.clear_pw(); }
  // Per-window sub stats for AerialVision support
  void get_sub_stats_pw(struct cache_sub_stats_pw &css) const {
    m_stats.get_sub_stats_pw(css);
  }

  // accessors for cache bandwidth availability
  bool data_port_free() const {
    return m_bandwidth_management.data_port_free();
  }
  bool fill_port_free() const {
    return m_bandwidth_management.fill_port_free();
  }

  // This is a gapping hole we are poking in the system to quickly handle
  // filling the cache on cudamemcopies. We don't care about anything other than
  // L2 state after the memcopy - so just force the tag array to act as though
  // something is read or written without doing anything else.
  void force_tag_access(new_addr_type addr, unsigned time,
                        mem_access_sector_mask_t mask) {
    if(m_config.m_cache_type != ELASTIC)
    	m_tag_array->fill(addr, time, mask);
    else if(m_config.m_cache_type == ELASTIC)
    	m_elastic_tag_array->fill(addr, time, mask);  //Elastic: Added 
  }

 protected:
  // Constructor that can be used by derived classes with custom tag arrays
   // Elastic:Added instantiation of l1_tag_array
  baseline_cache(const char *name, cache_config &config, int core_id,
                 int type_id, mem_fetch_interface *memport,
                 enum mem_fetch_status status,elastic_tag_array *new_elastic_tag_array)
      : m_config(config),
        m_elastic_tag_array(new_elastic_tag_array),
        m_mshrs(config.m_mshr_entries, config.m_mshr_max_merge),
        m_bandwidth_management(config) {
  	assert(m_config.m_cache_type == ELASTIC);
    init(name, config, memport, status);

  }
    //Elastic:Retained old one for read_only_cache use

  baseline_cache(const char *name, cache_config &config, int core_id,
                 int type_id, mem_fetch_interface *memport,
                 enum mem_fetch_status status, tag_array *new_tag_array)
      : m_config(config),
        m_tag_array(new_tag_array),
        m_mshrs(config.m_mshr_entries, config.m_mshr_max_merge),
        m_bandwidth_management(config) {
	assert(m_config.m_cache_type !=ELASTIC);
    init(name, config, memport, status);
  }

 protected:
  std::string m_name;
  cache_config &m_config;
  tag_array *m_tag_array;
  elastic_tag_array *m_elastic_tag_array; //Elastic:Added instantiation of l1_tag_array
  mshr_table m_mshrs;
  std::list<mem_fetch *> m_miss_queue;
  enum mem_fetch_status m_miss_queue_status;
  mem_fetch_interface *m_memport;

  struct extra_mf_fields {
    extra_mf_fields() { m_valid = false; }
    extra_mf_fields(new_addr_type a, new_addr_type ad, unsigned i,unsigned d,
                    const cache_config &m_config) {
      m_valid = true;
      m_block_addr = a;
      m_addr = ad;
      m_cache_index = i;
      m_word_index = (ad>>5)&0x3;
      m_data_size = d;
      switch(m_config.m_mshr_type){
      	case SECTOR_ASSOC:
      	  pending_read = m_config.m_line_sz / SECTOR_SIZE;
      	break;
      	case ELASTIC_ASSOC: //Elastic:Assumed we will never provide this config
      	  pending_read = m_config.m_line_sz / 32; //Elastic: come back later to configure
      	break;
      	default:
      	  pending_read = 0;
      	break;
      }

      } //Elastic:Added
        extra_mf_fields(new_addr_type a, new_addr_type ad, unsigned i,unsigned w, unsigned d,
                    const cache_config &m_config) {
      m_valid = true;
      m_block_addr = a;
      m_addr = ad;
      m_cache_index = i;
      m_word_index = w;
      m_data_size = d;
      switch(m_config.m_mshr_type){
      	case SECTOR_ASSOC:
      	  pending_read = m_config.m_line_sz / SECTOR_SIZE;
      	break;
      	case ELASTIC_ASSOC: //Elastic:Assumed we will never provide this config
      	  pending_read = m_config.m_line_sz / 32; //Elastic: come back later to configure
      	break;
      	default:
      	  pending_read = 0;
      	break;
      }

      }

    bool m_valid;
    new_addr_type m_block_addr;
    new_addr_type m_addr;
    unsigned m_cache_index;//Elastic:Added
    unsigned m_word_index;
    unsigned m_data_size;
    // this variable is used when a load request generates multiple load
    // transactions For example, a read request from non-sector L1 request sends
    // a request to sector L2
    unsigned pending_read;
  };

  typedef std::map<mem_fetch *, extra_mf_fields> extra_mf_fields_lookup;

  extra_mf_fields_lookup m_extra_mf_fields;

  cache_stats m_stats;

  /// Checks whether this request can be handled on this cycle. num_miss equals
  /// max # of misses to be handled on this cycle
  bool miss_queue_full(unsigned num_miss) {
    return ((m_miss_queue.size() + num_miss) >= m_config.m_miss_queue_size);
  }
  /// Read miss handler without writeback
  void send_read_request(new_addr_type addr, new_addr_type block_addr,
                         unsigned cache_index, mem_fetch *mf, unsigned time,
                         bool &do_miss, std::list<cache_event> &events,
                         bool read_only, bool wa);
  /// Read miss handler. Check MSHR hit or MSHR available
  void send_read_request(new_addr_type addr, new_addr_type block_addr,
                         unsigned cache_index, mem_fetch *mf, unsigned time,
                         bool &do_miss, bool &wb, evicted_block_info &evicted,
                         std::list<cache_event> &events, bool read_only,
                         bool wa);
/// Read miss handler without writeback
  //Elastic: Modified below two, added word_index and evicted_word_info
  void send_read_request_elastic(new_addr_type addr, new_addr_type block_addr,
                         unsigned cache_index,unsigned word_index,mem_fetch *mf, unsigned time,
                         bool &do_miss, std::list<cache_event> &events,bool read_only, bool wa);
  /// Read miss handler. Check MSHR hit or MSHR available
  void send_read_request_elastic(new_addr_type addr, new_addr_type block_addr,
                         unsigned cache_index,unsigned word_index, mem_fetch *mf, unsigned time,
                         bool &do_miss, bool &wb, evicted_block_info &evicted_line, evicted_word_info &evicted_word,
                         std::list<cache_event> &events, bool read_only,bool wa);


  /// Sub-class containing all metadata for port bandwidth management
  class bandwidth_management {
   public:
    bandwidth_management(cache_config &config);

    /// use the data port based on the outcome and events generated by the
    /// mem_fetch request
    void use_data_port(mem_fetch *mf, enum cache_request_status outcome,
                       const std::list<cache_event> &events);
    //Elastic:added
    void use_data_port_elastic(mem_fetch *mf, enum cache_request_status outcome,
                       const std::list<cache_event> &events);

    /// use the fill port
    void use_fill_port(mem_fetch *mf);

    /// called every cache cycle to free up the ports
    void replenish_port_bandwidth();

    /// query for data port availability
    bool data_port_free() const;
    /// query for fill port availability
    bool fill_port_free() const;

   protected:
    const cache_config &m_config;

    int m_data_port_occupied_cycles;  //< Number of cycle that the data port
                                      // remains used
    int m_fill_port_occupied_cycles;  //< Number of cycle that the fill port
                                      // remains used
  };

  bandwidth_management m_bandwidth_management;
  bool m_elastic_true; //Elastic:Added
};

/// Read only cache
class read_only_cache : public baseline_cache {
 public:
  read_only_cache(const char *name, cache_config &config, int core_id,
                  int type_id, mem_fetch_interface *memport,
                  enum mem_fetch_status status)
      : baseline_cache(name, config, core_id, type_id, memport, status) {}

  /// Access cache for read_only_cache: returns RESERVATION_FAIL if request
  /// could not be accepted (for any reason)
  virtual enum cache_request_status access(new_addr_type addr, mem_fetch *mf,
                                           unsigned time,
                                           std::list<cache_event> &events);

  virtual ~read_only_cache() {}

 protected:
  read_only_cache(const char *name, cache_config &config, int core_id,
                  int type_id, mem_fetch_interface *memport,
                  enum mem_fetch_status status, tag_array *new_tag_array)
      : baseline_cache(name, config, core_id, type_id, memport, status,
                       new_tag_array) {}
};

/// Data cache - Implements common functions for L1 and L2 data cache
class data_cache : public baseline_cache {
 public:
  //Elastic: Added only for l1 elastic use
  data_cache(const char *name, cache_config &config, int core_id, int type_id,
             mem_fetch_interface *memport, mem_fetch_allocator *mfcreator,
             enum mem_fetch_status status, mem_access_type wr_alloc_type,
             mem_access_type wrbk_type, class gpgpu_sim *gpu,bool elastic_true)
      : baseline_cache(name, config, core_id, type_id, memport, status,elastic_true) {
    assert(m_config.m_cache_type == ELASTIC);
    init(mfcreator);
    m_wr_alloc_type = wr_alloc_type;
    m_wrbk_type = wrbk_type;
    m_gpu = gpu;
    m_elastic_true = elastic_true;
  }
   //Elastic: Retained for l2 use or l1 non elastic use
  data_cache(const char *name, cache_config &config, int core_id, int type_id,
             mem_fetch_interface *memport, mem_fetch_allocator *mfcreator,
             enum mem_fetch_status status, mem_access_type wr_alloc_type,
             mem_access_type wrbk_type, class gpgpu_sim *gpu)
      : baseline_cache(name, config, core_id, type_id, memport, status) {
    assert(m_config.m_cache_type !=ELASTIC);
    init(mfcreator);
    m_wr_alloc_type = wr_alloc_type;
    m_wrbk_type = wrbk_type;
    m_gpu = gpu;
    m_elastic_true = false;//Elastic:Added
  }

  virtual ~data_cache() {}

  virtual void init(mem_fetch_allocator *mfcreator) {
    m_memfetch_creator = mfcreator;

    // Set read hit function
    m_rd_hit = &data_cache::rd_hit_base;
    m_rd_e_hit =  &data_cache::rd_hit_e_base;

    // Set read miss function
    m_rd_miss = &data_cache::rd_miss_base;
    m_rd_e_miss = &data_cache::rd_miss_e_base;


    // Set write hit function
    switch (m_config.m_write_policy) {
      // READ_ONLY is now a separate cache class, config is deprecated
      case READ_ONLY:
        assert(0 && "Error: Writable Data_cache set as READ_ONLY\n");
        break;
      case WRITE_BACK:
        m_wr_hit = &data_cache::wr_hit_wb;
         m_wr_e_hit = &data_cache::wr_hit_e_wb;
	 break;
      case WRITE_THROUGH:
        m_wr_hit = &data_cache::wr_hit_wt;
        m_wr_e_hit = &data_cache::wr_hit_e_wt;
        break;
      case WRITE_EVICT:
        m_wr_hit = &data_cache::wr_hit_we;
        m_wr_e_hit = &data_cache::wr_hit_e_we;
        break;
      case LOCAL_WB_GLOBAL_WT:
        m_wr_hit = &data_cache::wr_hit_global_we_local_wb;
        m_wr_e_hit = &data_cache::wr_hit_global_we_local_e_wb;
        break;
      default:
        assert(0 && "Error: Must set valid cache write policy\n");
        break;  // Need to set a write hit function
    }

    // Set write miss function
    switch (m_config.m_write_alloc_policy) {
      case NO_WRITE_ALLOCATE:
        m_wr_miss = &data_cache::wr_miss_no_wa;
        m_wr_e_miss = &data_cache::wr_miss_no_e_wa;
        break;
      case WRITE_ALLOCATE:
        m_wr_miss = &data_cache::wr_miss_wa_naive;
         m_wr_e_miss = &data_cache::wr_miss_wa_e_naive;
        break;
      case FETCH_ON_WRITE:
        m_wr_miss = &data_cache::wr_miss_wa_fetch_on_write;
        m_wr_e_miss = &data_cache::wr_miss_wa_fetch_on_e_write;
        break;
      case LAZY_FETCH_ON_READ:
        m_wr_miss = &data_cache::wr_miss_wa_lazy_fetch_on_read;
        m_wr_e_miss = &data_cache::wr_miss_wa_lazy_fetch_on_e_read;
        break;
      default:
        assert(0 && "Error: Must set valid cache write miss policy\n");
        break;  // Need to set a write miss function
    }
  }
 //Elastic:Added 
  virtual enum cache_request_status access_elastic(new_addr_type addr, mem_fetch *mf,
                                           unsigned time,
                                           std::list<cache_event> &events);

  virtual enum cache_request_status access(new_addr_type addr, mem_fetch *mf,
                                           unsigned time,
                                           std::list<cache_event> &events);

 protected:
  //Elastic: Added for l1 use(also note the extra arg new_l1_tag_array) in ELASTIC mode only
  data_cache(const char *name, cache_config &config, int core_id, int type_id,
             mem_fetch_interface *memport, mem_fetch_allocator *mfcreator,
             enum mem_fetch_status status, elastic_tag_array *new_elastic_tag_array,
             mem_access_type wr_alloc_type, mem_access_type wrbk_type,
             class gpgpu_sim *gpu,bool elastic_true)
      : baseline_cache(name, config, core_id, type_id, memport, status,
                       new_elastic_tag_array) {
    assert(m_config.m_cache_type == ELASTIC);
    init(mfcreator);
    m_wr_alloc_type = wr_alloc_type;
    m_wrbk_type = wrbk_type;
    m_gpu = gpu;
  }
   //Elastic: Retained for l2 use or l1 non elastic use
 data_cache(const char *name, cache_config &config, int core_id, int type_id,
             mem_fetch_interface *memport, mem_fetch_allocator *mfcreator,
             enum mem_fetch_status status, tag_array *new_tag_array,
             mem_access_type wr_alloc_type, mem_access_type wrbk_type,
             class gpgpu_sim *gpu)
      : baseline_cache(name, config, core_id, type_id, memport, status,
                       new_tag_array) {
    assert(m_config.m_cache_type!=ELASTIC);
    init(mfcreator);
    m_wr_alloc_type = wr_alloc_type;
    m_wrbk_type = wrbk_type;
    m_gpu = gpu;
  }

  mem_access_type m_wr_alloc_type;  // Specifies type of write allocate request
                                    // (e.g., L1 or L2)
  mem_access_type
      m_wrbk_type;  // Specifies type of writeback request (e.g., L1 or L2)
  class gpgpu_sim *m_gpu;

  //! A general function that takes the result of a tag_array probe
  //  and performs the correspding functions based on the cache configuration
  //  The access fucntion calls this function
//Elastic: Added word_index and elastic event
  enum cache_request_status process_tag_probe_elastic(bool wr,
                                              enum cache_request_status status,
                                              new_addr_type addr,
                                              unsigned cache_index,unsigned word_index,
                                              mem_fetch *mf, unsigned time,
                                              std::list<cache_event> &events);
  
enum cache_request_status process_tag_probe(bool wr,
                                              enum cache_request_status status,
                                              new_addr_type addr,
                                              unsigned cache_index,
                                              mem_fetch *mf, unsigned time,
                                              std::list<cache_event> &events);

 protected:
  mem_fetch_allocator *m_memfetch_creator;

  // Functions for data cache access
  /// Sends write request to lower level memory (write or writeback)
  //Elastic:added elastic request and elastic event
  void send_write_request_elastic(mem_fetch *mf, cache_event request ,unsigned time,
                          std::list<cache_event> &events);

  void send_write_request(mem_fetch *mf, cache_event request, unsigned time,
                          std::list<cache_event> &events);

// Member Function pointers - Set by configuration options
  // to the functions below each grouping
  //Elastic: Added word_index as extra args to all event functions below
  /******* Write-hit configs *******/
  enum cache_request_status (data_cache::*m_wr_e_hit)(
      new_addr_type addr, unsigned cache_index,unsigned word_index, mem_fetch *mf, unsigned time,
       std::list<cache_event> &events,enum cache_request_status status);
  /// Marks block as MODIFIED and updates block LRU
  enum cache_request_status wr_hit_e_wb(
      new_addr_type addr, unsigned cache_index,unsigned word_index, mem_fetch *mf, unsigned time,
       std::list<cache_event> &events,
      enum cache_request_status status);  // write-back
  enum cache_request_status wr_hit_e_wt(
      new_addr_type addr, unsigned cache_index, unsigned word_index,mem_fetch *mf, unsigned time,
       std::list<cache_event> &events,
      enum cache_request_status status); // write-through

  /// Marks block as INVALID and sends write request to lower level memory
  enum cache_request_status wr_hit_e_we(
      new_addr_type addr, unsigned cache_index,unsigned word_index, mem_fetch *mf, unsigned time,
       std::list<cache_event> &events,
      enum cache_request_status status);  // write-evict
  enum cache_request_status wr_hit_global_we_local_e_wb(
      new_addr_type addr, unsigned cache_index, unsigned word_index,mem_fetch *mf, unsigned time,
       std::list<cache_event> &events, enum cache_request_status status);
  // global write-evict, local write-back

  /******* Write-miss configs *******/
  enum cache_request_status (data_cache::*m_wr_e_miss)(
      new_addr_type addr, unsigned cache_index, unsigned word_index,mem_fetch *mf, unsigned time,
        std::list<cache_event> &events,enum cache_request_status status);
  /// Sends read request, and possible write-back request,
  //  to lower level memory for a write miss with write-allocate
  enum cache_request_status wr_miss_wa_e_naive(
      new_addr_type addr, unsigned cache_index, unsigned word_index,mem_fetch *mf, unsigned time,
       std::list<cache_event> &events,
      enum cache_request_status
          status);  // write-allocate-send-write-and-read-request
  enum cache_request_status wr_miss_wa_fetch_on_e_write(
      new_addr_type addr, unsigned cache_index, unsigned word_index,mem_fetch *mf, unsigned time,
       std::list<cache_event> &events,
      enum cache_request_status
          status);  // write-allocate with fetch-on-every-write
  enum cache_request_status wr_miss_wa_lazy_fetch_on_e_read(
      new_addr_type addr, unsigned cache_index, unsigned word_index,mem_fetch *mf, unsigned time,
       std::list<cache_event> &events,
      enum cache_request_status status);  // write-allocate with read-fetch-only
  enum cache_request_status wr_miss_wa_write_e_validate(
      new_addr_type addr, unsigned cache_index, unsigned word_index,mem_fetch *mf, unsigned time,
       std::list<cache_event> &events,
      enum cache_request_status
          status);  // write-allocate that writes with no read fetch
  enum cache_request_status wr_miss_no_e_wa(
      new_addr_type addr, unsigned cache_index, unsigned word_index,mem_fetch *mf, unsigned time,
       std::list<cache_event> &events,
      enum cache_request_status status); // no write-allocate

  // Currently no separate functions for reads
  /******* Read-hit configs *******/
  enum cache_request_status (data_cache::*m_rd_e_hit)(
      new_addr_type addr, unsigned cache_index, unsigned word_index,mem_fetch *mf, unsigned time,
        std::list<cache_event> &events,enum cache_request_status status);
  enum cache_request_status rd_hit_e_base(new_addr_type addr,
                                        unsigned cache_index, unsigned word_index,mem_fetch *mf,
                                        unsigned time,
                                         std::list<cache_event> &events,
                                        enum cache_request_status status);

  /******* Read-miss configs *******/
  enum cache_request_status (data_cache::*m_rd_e_miss)(
      new_addr_type addr, unsigned cache_index,unsigned word_index, mem_fetch *mf, unsigned time,
       std::list<cache_event> &events,enum cache_request_status status);
  enum cache_request_status rd_miss_e_base(new_addr_type addr,
                                         unsigned cache_index, unsigned word_index,mem_fetch *mf,
                                         unsigned time,
                                          std::list<cache_event> &events,
                                         enum cache_request_status status);
 
 // Member Function pointers - Set by configuration options
  // to the functions below each grouping
  /******* Write-hit configs *******/
  enum cache_request_status (data_cache::*m_wr_hit)(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events, enum cache_request_status status);
  /// Marks block as MODIFIED and updates block LRU
  enum cache_request_status wr_hit_wb(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events,
      enum cache_request_status status);  // write-back
  enum cache_request_status wr_hit_wt(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events,
      enum cache_request_status status);  // write-through

  /// Marks block as INVALID and sends write request to lower level memory
  enum cache_request_status wr_hit_we(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events,
      enum cache_request_status status);  // write-evict
  enum cache_request_status wr_hit_global_we_local_wb(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events, enum cache_request_status status);
  // global write-evict, local write-back

  /******* Write-miss configs *******/
  enum cache_request_status (data_cache::*m_wr_miss)(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events, enum cache_request_status status);
  /// Sends read request, and possible write-back request,
  //  to lower level memory for a write miss with write-allocate
  enum cache_request_status wr_miss_wa_naive(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events,
      enum cache_request_status
          status);  // write-allocate-send-write-and-read-request
  enum cache_request_status wr_miss_wa_fetch_on_write(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events,
      enum cache_request_status
          status);  // write-allocate with fetch-on-every-write
  enum cache_request_status wr_miss_wa_lazy_fetch_on_read(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events,
      enum cache_request_status status);  // write-allocate with read-fetch-only
  enum cache_request_status wr_miss_wa_write_validate(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events,
      enum cache_request_status
          status);  // write-allocate that writes with no read fetch
  enum cache_request_status wr_miss_no_wa(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events,
      enum cache_request_status status);  // no write-allocate

  // Currently no separate functions for reads
  /******* Read-hit configs *******/
  enum cache_request_status (data_cache::*m_rd_hit)(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events, enum cache_request_status status);
  enum cache_request_status rd_hit_base(new_addr_type addr,
                                        unsigned cache_index, mem_fetch *mf,
                                        unsigned time,
                                        std::list<cache_event> &events,
                                        enum cache_request_status status);

  /******* Read-miss configs *******/
  enum cache_request_status (data_cache::*m_rd_miss)(
      new_addr_type addr, unsigned cache_index, mem_fetch *mf, unsigned time,
      std::list<cache_event> &events, enum cache_request_status status);
  enum cache_request_status rd_miss_base(new_addr_type addr,
                                         unsigned cache_index, mem_fetch *mf,
                                         unsigned time,
                                         std::list<cache_event> &events,
                                         enum cache_request_status status);
};

/// This is meant to model the first level data cache in Fermi.
/// It is write-evict (global) or write-back (local) at
/// the granularity of individual blocks
/// (the policy used in fermi according to the CUDA manual)
class l1_cache : public data_cache {
 public:
  l1_cache(const char *name, cache_config &config, int core_id, int type_id,
           mem_fetch_interface *memport, mem_fetch_allocator *mfcreator,
           enum mem_fetch_status status, class gpgpu_sim *gpu)
      : data_cache(name, config, core_id, type_id, memport, mfcreator, status,
                   L1_WR_ALLOC_R, L1_WRBK_ACC, gpu) {
assert(m_config.m_cache_type !=ELASTIC);
}

  virtual ~l1_cache() {}

  virtual enum cache_request_status access(new_addr_type addr, mem_fetch *mf,
                                           unsigned time,
                                           std::list<cache_event> &events);

 protected:
  l1_cache(const char *name, cache_config &config, int core_id, int type_id,
           mem_fetch_interface *memport, mem_fetch_allocator *mfcreator,
           enum mem_fetch_status status, tag_array *new_tag_array,
           class gpgpu_sim *gpu)
      : data_cache(name, config, core_id, type_id, memport, mfcreator, status,
                   new_tag_array, L1_WR_ALLOC_R, L1_WRBK_ACC, gpu) {}
};
//Elastic: elastic l1 cache 
class l1_elastic_cache : public data_cache {
 public:
  l1_elastic_cache(const char *name, cache_config &config, int core_id, int type_id,
           mem_fetch_interface *memport, mem_fetch_allocator *mfcreator,
           enum mem_fetch_status status, class gpgpu_sim *gpu)
      : data_cache(name, config, core_id, type_id, memport, mfcreator, status,
                   L1_WR_ALLOC_R, L1_WRBK_ACC, gpu,true) {
   	assert(m_config.m_cache_type == ELASTIC);
   }

  virtual ~l1_elastic_cache() {}

  virtual enum cache_request_status access(new_addr_type addr, mem_fetch *mf,
                                           unsigned time,
                                           std::list<cache_event> &events);

 protected:
  l1_elastic_cache(const char *name, cache_config &config, int core_id, int type_id,
           mem_fetch_interface *memport, mem_fetch_allocator *mfcreator,
           enum mem_fetch_status status, tag_array *new_tag_array,elastic_tag_array *new_elastic_tag_array
           ,class gpgpu_sim *gpu)
      : data_cache(name, config, core_id, type_id, memport, mfcreator, status,
                   new_elastic_tag_array,L1_WR_ALLOC_R, L1_WRBK_ACC, gpu,true) {
   	    assert(m_config.m_cache_type == ELASTIC);
		}
};


/// Models second level shared cache with global write-back
/// and write-allocate policies
class l2_cache : public data_cache {
 public:
  l2_cache(const char *name, cache_config &config, int core_id, int type_id,
           mem_fetch_interface *memport, mem_fetch_allocator *mfcreator,
           enum mem_fetch_status status, class gpgpu_sim *gpu)
      : data_cache(name, config, core_id, type_id, memport, mfcreator, status,
                   L2_WR_ALLOC_R, L2_WRBK_ACC, gpu) {
assert(m_config.m_cache_type!=ELASTIC);
}

  virtual ~l2_cache() {}

  virtual enum cache_request_status access(new_addr_type addr, mem_fetch *mf,
                                           unsigned time,
                                           std::list<cache_event> &events);
};
class l2_elastic_cache : public data_cache {
 public:
  l2_elastic_cache(const char *name, cache_config &config, int core_id, int type_id,
           mem_fetch_interface *memport, mem_fetch_allocator *mfcreator,
           enum mem_fetch_status status, class gpgpu_sim *gpu)
      : data_cache(name, config, core_id, type_id, memport, mfcreator, status,
                   L2_WR_ALLOC_R, L2_WRBK_ACC, gpu,true) {
assert(m_config.m_cache_type==ELASTIC);
}

  virtual ~l2_elastic_cache() {}

  virtual enum cache_request_status access(new_addr_type addr, mem_fetch *mf,
                                           unsigned time,
                                           std::list<cache_event> &events);
};


/*****************************************************************************/

// See the following paper to understand this cache model:
//
// Igehy, et al., Prefetching in a Texture Cache Architecture,
// Proceedings of the 1998 Eurographics/SIGGRAPH Workshop on Graphics Hardware
// http://www-graphics.stanford.edu/papers/texture_prefetch/
class tex_cache : public cache_t {
 public:
  tex_cache(const char *name, cache_config &config, int core_id, int type_id,
            mem_fetch_interface *memport, enum mem_fetch_status request_status,
            enum mem_fetch_status rob_status)
      : m_config(config),
        m_tags(config, core_id, type_id),
        m_fragment_fifo(config.m_fragment_fifo_entries),
        m_request_fifo(config.m_request_fifo_entries),
        m_rob(config.m_rob_entries),
        m_result_fifo(config.m_result_fifo_entries) {
    m_name = name;
    assert(config.m_mshr_type == TEX_FIFO ||
           config.m_mshr_type == SECTOR_TEX_FIFO);
    assert(config.m_write_policy == READ_ONLY);
    assert(config.m_alloc_policy == ON_MISS);
    m_memport = memport;
    m_cache = new data_block[config.get_num_lines()];
    m_request_queue_status = request_status;
    m_rob_status = rob_status;
  }

  /// Access function for tex_cache
  /// return values: RESERVATION_FAIL if request could not be accepted
  /// otherwise returns HIT_RESERVED or MISS; NOTE: *never* returns HIT
  /// since unlike a normal CPU cache, a "HIT" in texture cache does not
  /// mean the data is ready (still need to get through fragment fifo)
  enum cache_request_status access(new_addr_type addr, mem_fetch *mf,
                                   unsigned time,
                                   std::list<cache_event> &events);
  void cycle();
  /// Place returning cache block into reorder buffer
  void fill(mem_fetch *mf, unsigned time);
  /// Are any (accepted) accesses that had to wait for memory now ready? (does
  /// not include accesses that "HIT")
  bool access_ready() const { return !m_result_fifo.empty(); }
  /// Pop next ready access (includes both accesses that "HIT" and those that
  /// "MISS")
  mem_fetch *next_access() { return m_result_fifo.pop(); }
  void display_state(FILE *fp) const;

  // accessors for cache bandwidth availability - stubs for now
  bool data_port_free() const { return true; }
  bool fill_port_free() const { return true; }

  // Stat collection
  const cache_stats &get_stats() const { return m_stats; }
  unsigned get_stats(enum mem_access_type *access_type,
                     unsigned num_access_type,
                     enum cache_request_status *access_status,
                     unsigned num_access_status) const {
    return m_stats.get_stats(access_type, num_access_type, access_status,
                             num_access_status);
  }

  void get_sub_stats(struct cache_sub_stats &css) const {
    m_stats.get_sub_stats(css);
  }

 private:
  std::string m_name;
  const cache_config &m_config;

  struct fragment_entry {
    fragment_entry() {}
    fragment_entry(mem_fetch *mf, unsigned idx, bool m, unsigned d) {
      m_request = mf;
      m_cache_index = idx;
      m_miss = m;
      m_data_size = d;
    }
    mem_fetch *m_request;    // request information
    unsigned m_cache_index;  // where to look for data
    bool m_miss;             // true if sent memory request
    unsigned m_data_size;
  };

  struct rob_entry {
    rob_entry() {
      m_ready = false;
      m_time = 0;
      m_request = NULL;
    }
    rob_entry(unsigned i, mem_fetch *mf, new_addr_type a) {
      m_ready = false;
      m_index = i;
      m_time = 0;
      m_request = mf;
      m_block_addr = a;
    }
    bool m_ready;
    unsigned m_time;   // which cycle did this entry become ready?
    unsigned m_index;  // where in cache should block be placed?
    mem_fetch *m_request;
    new_addr_type m_block_addr;
  };

  struct data_block {
    data_block() { m_valid = false; }
    bool m_valid;
    new_addr_type m_block_addr;
  };

  // TODO: replace fifo_pipeline with this?
  template <class T>
  class fifo {
   public:
    fifo(unsigned size) {
      m_size = size;
      m_num = 0;
      m_head = 0;
      m_tail = 0;
      m_data = new T[size];
    }
    bool full() const { return m_num == m_size; }
    bool empty() const { return m_num == 0; }
    unsigned size() const { return m_num; }
    unsigned capacity() const { return m_size; }
    unsigned push(const T &e) {
      assert(!full());
      m_data[m_head] = e;
      unsigned result = m_head;
      inc_head();
      return result;
    }
    T pop() {
      assert(!empty());
      T result = m_data[m_tail];
      inc_tail();
      return result;
    }
    const T &peek(unsigned index) const {
      assert(index < m_size);
      return m_data[index];
    }
    T &peek(unsigned index) {
      assert(index < m_size);
      return m_data[index];
    }
    T &peek() const { return m_data[m_tail]; }
    unsigned next_pop_index() const { return m_tail; }

   private:
    void inc_head() {
      m_head = (m_head + 1) % m_size;
      m_num++;
    }
    void inc_tail() {
      assert(m_num > 0);
      m_tail = (m_tail + 1) % m_size;
      m_num--;
    }

    unsigned m_head;  // next entry goes here
    unsigned m_tail;  // oldest entry found here
    unsigned m_num;   // how many in fifo?
    unsigned m_size;  // maximum number of entries in fifo
    T *m_data;
  };

  tag_array m_tags;
  fifo<fragment_entry> m_fragment_fifo;
  fifo<mem_fetch *> m_request_fifo;
  fifo<rob_entry> m_rob;
  data_block *m_cache;
  fifo<mem_fetch *> m_result_fifo;  // next completed texture fetch

  mem_fetch_interface *m_memport;
  enum mem_fetch_status m_request_queue_status;
  enum mem_fetch_status m_rob_status;

  struct extra_mf_fields {
    extra_mf_fields() { m_valid = false; }
    extra_mf_fields(unsigned i, const cache_config &m_config) {
      m_valid = true;
      m_rob_index = i;
      pending_read = m_config.m_mshr_type == SECTOR_TEX_FIFO
                         ? m_config.m_line_sz / SECTOR_SIZE
                         : 0;
    }
    bool m_valid;
    unsigned m_rob_index;
    unsigned pending_read;
  };

  cache_stats m_stats;

  typedef std::map<mem_fetch *, extra_mf_fields> extra_mf_fields_lookup;

  extra_mf_fields_lookup m_extra_mf_fields;
};

#endif
