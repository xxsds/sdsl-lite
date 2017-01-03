// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#ifndef INCLUDED_SDSL_MEMORY_HUGEPAGES
#define INCLUDED_SDSL_MEMORY_HUGEPAGES

#include "uintx_t.hpp"
#include "config.hpp"
#include "bits.hpp"
#include "memory_tracking.hpp"

#include <chrono>

using namespace std::chrono;

namespace sdsl {

#pragma pack(push, 1)
typedef struct mm_block {
	size_t			 size;
	struct mm_block* next;
	struct mm_block* prev;
} mm_block_t;

typedef struct bfoot {
	size_t size;
} mm_block_foot_t;
#pragma pack(pop)

#define ALIGNMENT sizeof(uint64_t)
#define ALIGNSPLIT(size) (((size)) & ~0x7)
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define MM_BLOCK_OVERHEAD (sizeof(size_t) + sizeof(size_t))
#define MIN_BLOCKSIZE (ALIGN(sizeof(mm_block_t) + sizeof(mm_block_foot_t)))
#define UNMASK_SIZE(size) ((size) & ~1)
#define ISFREE(size) ((size)&1)
#define SETFREE(size) ((size) | 1)
#define SPLIT_THRESHOLD (MIN_BLOCKSIZE)

#ifndef MSVC_COMPILER
class hugepage_allocator {
private:
	uint8_t*	m_base		  = nullptr;
	mm_block_t* m_first_block = nullptr;
	uint8_t*	m_top		  = nullptr;
	size_t		m_total_size  = 0;
	std::multimap<size_t, mm_block_t*> m_free_large;

private:
	uint64_t extract_number(std::string& line)
	{
		std::string num_str;
		for (size_t i = line.size() - 1; i + 1 >= 1; i--) {
			if (isdigit(line[i])) {
				num_str.insert(num_str.begin(), line[i]);
			} else {
				if (num_str.size() > 0) {
					break;
				}
			}
		}
		return std::strtoull(num_str.c_str(), nullptr, 10);
	}


	uint64_t extract_multiplier(std::string& line)
	{
		uint64_t num = 1;
		if (line[line.size() - 2] == 'k' || line[line.size() - 2] == 'K') {
			num = 1024;
		}
		if (line[line.size() - 2] == 'm' || line[line.size() - 2] == 'M') {
			num = 1024 * 1024;
		}
		if (line[line.size() - 2] == 'g' || line[line.size() - 2] == 'G') {
			num = 1024 * 1024 * 1024;
		}
		return num;
	}


	size_t determine_available_hugepage_memory()
	{
		size_t			  size_in_bytes		 = 0;
		size_t			  page_size_in_bytes = 0;
		size_t			  num_free_pages	 = 0;
		const std::string meminfo_file		 = "/proc/meminfo";
		const std::string ps_str			 = "Hugepagesize:";
		const std::string pf_str			 = "HugePages_Free:";
		std::ifstream	 mifs(meminfo_file);
		if (mifs.is_open()) {
			// find size of one page
			std::string line;
			while (std::getline(mifs, line)) {
				auto ps = std::mismatch(ps_str.begin(), ps_str.end(), line.begin());
				if (ps.first == ps_str.end()) {
					page_size_in_bytes = extract_number(line) * extract_multiplier(line);
				}
				auto pf = std::mismatch(pf_str.begin(), pf_str.end(), line.begin());
				if (pf.first == pf_str.end()) {
					num_free_pages = extract_number(line);
				}
			}
			size_in_bytes = page_size_in_bytes * num_free_pages;
		} else {
			throw std::system_error(
			ENOMEM,
			std::system_category(),
			"hugepage_allocator could not automatically determine available hugepages");
		}
		return size_in_bytes;
	}

	mm_block_t* block_cur(void* ptr)
	{
		mm_block_t* bptr = (mm_block_t*)((uint8_t*)ptr - sizeof(size_t));
		return bptr;
	}

	/* given a block retrieve the previous block if any. nullptr otherwise */
	mm_block_t* block_prev(mm_block_t* cur_bptr, mm_block_t* first)
	{
		/* start of the heap? */
		if (cur_bptr == first) return nullptr;
		mm_block_foot_t* prev_foot =
		(mm_block_foot_t*)((uint8_t*)cur_bptr - sizeof(mm_block_foot_t));
		mm_block_t* prev_bptr = (mm_block_t*)((uint8_t*)cur_bptr - UNMASK_SIZE(prev_foot->size));
		return prev_bptr;
	}

	/* given a block retrieve the next block if any. nullptr otherwise */
	mm_block_t* block_next(mm_block_t* cur_bptr, uint8_t* top)
	{
		/* end of the heap? */
		if ((uint8_t*)((uint8_t*)cur_bptr + UNMASK_SIZE(cur_bptr->size)) >= top) return nullptr;

		mm_block_t* next_bptr = (mm_block_t*)((uint8_t*)cur_bptr + UNMASK_SIZE(cur_bptr->size));
		return next_bptr;
	}

	/* calculate the size of a memory block */
	size_t block_size(void* ptr)
	{
		mm_block_t* bptr = block_cur(ptr);
		return UNMASK_SIZE(bptr->size);
	}

	bool block_isfree(mm_block_t* ptr)
	{
		;
		return ((ptr->size) & 1ULL);
	}

	/* is the next block free */
	bool block_nextfree(mm_block_t* ptr, uint8_t* top)
	{
		mm_block_t* next = block_next(ptr, top);
		if (next && block_isfree(next)) return true;
		return false;
	}

	/* is the prev block free */
	bool block_prevfree(mm_block_t* ptr, mm_block_t* begin)
	{
		mm_block_t* prev = block_prev(ptr, begin);
		if (prev && block_isfree(prev)) return 1;
		return 0;
	}

	/* update the footer with a new size */
	void foot_update(mm_block_t* ptr, size_t size)
	{
		mm_block_foot_t* fptr =
		(mm_block_foot_t*)((uint8_t*)ptr + UNMASK_SIZE(size) - sizeof(mm_block_foot_t));
		fptr->size = size;
	}

	/* update the block with a new size */
	void block_update(mm_block_t* ptr, size_t size)
	{
		ptr->size = size;
		foot_update(ptr, size);
	}

	/* return the pointer to the "data" */
	void* block_data(mm_block_t* ptr) { return (void*)((uint8_t*)ptr + sizeof(size_t)); }

	/* return size of the data that can be stored in the block */
	size_t block_getdatasize(mm_block_t* ptr)
	{
		size_t blocksize = UNMASK_SIZE(ptr->size);
		return blocksize - sizeof(size_t) - sizeof(mm_block_foot_t);
	}

	/* mark the block as free */
	void block_markfree(mm_block_t* ptr) { block_update(ptr, SETFREE(ptr->size)); }

	/* mark the block as used */
	void block_markused(mm_block_t* ptr) { block_update(ptr, UNMASK_SIZE(ptr->size)); }

	void coalesce_block(mm_block_t* block)
	{
		//std::cout << "coalesce_block()" << std::endl;
		mm_block_t* newblock = block;
		if (block_nextfree(block, m_top)) {
			mm_block_t* next = block_next(block, m_top);
			/* remove the "next" block from the free list */
			remove_from_free_set(next);
			/* add the size of our block */
			block_update(block, UNMASK_SIZE(block->size) + UNMASK_SIZE(next->size));
		}
		if (block_prevfree(block, m_first_block)) {
			mm_block_t* prev = block_prev(block, m_first_block);
			/* we remove the old prev block and readd it to the correct
           size list if necessary */
			remove_from_free_set(prev);
			newblock = prev;
			block_update(prev, UNMASK_SIZE(prev->size) + UNMASK_SIZE(block->size));
		}
		if (newblock) {
			block_markfree(newblock);
			insert_into_free_set(newblock);
		}
	}

	void split_block(mm_block_t* bptr, size_t size)
	{
		//std::cout << "split_block("<< (void*)bptr << ")" << std::endl;
		size_t blocksize = UNMASK_SIZE(bptr->size);
		//std::cout << "cur_block_size = " << blocksize << std::endl;
		/* only split if we get at least a small block
       out of it */
		int64_t newblocksize = ALIGNSPLIT(blocksize - ALIGN(size + MM_BLOCK_OVERHEAD));
		//std::cout << "new_block_size = " << newblocksize << std::endl;
		if (newblocksize >= (int64_t)SPLIT_THRESHOLD) {
			/* update blocksize of old block */
			//std::cout << "block_update = " << blocksize-newblocksize << std::endl;
			block_update(bptr, blocksize - newblocksize);
			mm_block_t* newblock = (mm_block_t*)((char*)bptr + (blocksize - newblocksize));
			//std::cout << "new block ptr = " << (void*)newblock << std::endl;
			block_update(newblock, newblocksize);
			coalesce_block(newblock);
		}
	}

	uint8_t* hsbrk(size_t size)
	{
		ptrdiff_t left = (ptrdiff_t)m_total_size - (m_top - m_base);
		if (left < (ptrdiff_t)size) { // enough space left?
			throw std::system_error(ENOMEM,
									std::system_category(),
									"hugepage_allocator: not enough hugepage memory available");
		}
		uint8_t* new_mem = m_top;
		m_top += size;
		return new_mem;
	}

	mm_block_t* new_block(size_t size)
	{
		//std::cout << "new_block(" << size << ")" << std::endl;
		size						   = ALIGN(size + MM_BLOCK_OVERHEAD);
		if (size < MIN_BLOCKSIZE) size = MIN_BLOCKSIZE;
		mm_block_t* ptr				   = (mm_block_t*)hsbrk(size);
		block_update(ptr, size);
		return ptr;
	}


	void remove_from_free_set(mm_block_t* block)
	{
		//std::cout << "remove_from_free_set()" << std::endl;
		auto eq_range = m_free_large.equal_range(block->size);
		// find the block amoung the blocks with equal size
		auto itr   = eq_range.first;
		auto last  = eq_range.second;
		auto found = m_free_large.end();
		while (itr != last) {
			if (itr->second == block) {
				found = itr;
			}
			++itr;
		}
		if (found == m_free_large.end()) {
			found = last;
		}
		m_free_large.erase(found);
	}

	void insert_into_free_set(mm_block_t* block) { m_free_large.insert({block->size, block}); }


	mm_block_t* find_free_block(size_t size_in_bytes)
	{
		mm_block_t* bptr	   = nullptr;
		auto		free_block = m_free_large.lower_bound(size_in_bytes);
		if (free_block != m_free_large.end()) {
			bptr = free_block->second;
			m_free_large.erase(free_block);
		}
		return bptr;
	}

	mm_block_t* last_block()
	{
		mm_block_t* last = nullptr;
		if (m_top != m_base) {
			mm_block_foot_t* fptr = (mm_block_foot_t*)(m_top - sizeof(size_t));
			last = (mm_block_t*)(((uint8_t*)fptr) - UNMASK_SIZE(fptr->size) + sizeof(size_t));
		}
		return last;
	}

	void block_print(int id, mm_block_t* bptr)
	{
		fprintf(stdout,
				"%d addr=%p size=%lu (%lu) free=%d\n",
				id,
				((void*)bptr),
				UNMASK_SIZE(bptr->size),
				bptr->size,
				block_isfree(bptr));
		fflush(stdout);
	}

	void print_heap()
	{
		mm_block_t* bptr = m_first_block;
		size_t		id   = 0;
		while (bptr) {
			block_print(id, bptr);
			id++;
			bptr = block_next(bptr, m_top);
		}
	}

public:
	void init(SDSL_UNUSED size_t size_in_bytes = 0)
	{
#ifdef MAP_HUGETLB
		if (size_in_bytes == 0) {
			size_in_bytes = determine_available_hugepage_memory();
		}

		m_total_size = size_in_bytes;
		m_base		 = (uint8_t*)mmap(nullptr,
								m_total_size,
								(PROT_READ | PROT_WRITE),
								(MAP_HUGETLB | MAP_ANONYMOUS | MAP_PRIVATE),
								0,
								0);
		if (m_base == MAP_FAILED) {
			throw std::system_error(
			ENOMEM, std::system_category(), "hugepage_allocator could not allocate hugepages");
		} else {
			// init the allocator
			m_top		  = m_base;
			m_first_block = (mm_block_t*)m_base;
		}
#else
		throw std::system_error(ENOMEM,
								std::system_category(),
								"hugepage_allocator: MAP_HUGETLB / hugepage support not available");
#endif
	}
	void* mm_realloc(void* ptr, size_t size)
	{
		if (nullptr == ptr) return mm_alloc(size);
		if (size == 0) {
			mm_free(ptr);
			return nullptr;
		}
		mm_block_t* bptr = block_cur(ptr);

		bool   need_malloc   = 0;
		size_t blockdatasize = block_getdatasize(bptr);
		/* we do nothing if the size is equal to the block */
		if (size == blockdatasize) {
			//std::cout << "return ptr = " << ptr << std::endl;
			return ptr; /* do nothing if size fits already */
		}
		if (size < blockdatasize) {
			/* we shrink */
			/* do we shrink enough to perform a split? */
			//std::cout << "shrink!" << std::endl;
			split_block(bptr, size);
		} else {
			//std::cout << "expand!" << std::endl;
			/* we expand */
			/* if the next block is free we could use it! */
			mm_block_t* next = block_next(bptr, m_top);
			if (!next) {
				//std::cout << "no next! -> expand!" << std::endl;
				// we are the last block so we just expand
				blockdatasize = block_getdatasize(bptr);
				size_t needed = ALIGN(size - blockdatasize);
				hsbrk(needed);
				block_update(bptr, UNMASK_SIZE(bptr->size) + needed);
				return block_data(bptr);
			} else {
				// we are not the last block
				//std::cout << "try combine next" << std::endl;
				if (next && block_isfree(next)) {
					/* do we have enough space if we use the next block */
					if (blockdatasize + UNMASK_SIZE(next->size) >= size) {
						/* the next block is enough! */
						/* remove the "next" block from the free list */
						remove_from_free_set(next);
						/* add the size of our block */
						block_update(bptr, UNMASK_SIZE(bptr->size) + UNMASK_SIZE(next->size));
					} else {
						/* the next block is not enough. we allocate a new one instead */
						need_malloc = true;
					}
				} else {
					/* try combing the previous block if free */
					//std::cout << "try combine prev" << std::endl;
					mm_block_t* prev = block_prev(bptr, m_first_block);
					if (prev && block_isfree(prev)) {
						if (blockdatasize + UNMASK_SIZE(prev->size) >= size) {
							remove_from_free_set(prev);
							size_t newsize = UNMASK_SIZE(prev->size) + UNMASK_SIZE(bptr->size);
							block_update(prev, newsize);
							block_markused(prev);
							/* move the data into the previous block */
							ptr = memmove(block_data(prev), ptr, blockdatasize);
						} else {
							/* not enough in the prev block */
							need_malloc = true;
						}
					} else {
						/* prev block not free. get more memory */
						need_malloc = true;
					}
				}
			}
		}
		if (need_malloc) {
			//std::cout << "need_alloc in REALLOC!" << std::endl;
			void* newptr = mm_alloc(size);
			memcpy(newptr, ptr, size);
			mm_free(ptr);
			ptr = newptr;
		}
		//print_heap();
		//std::cout << "return ptr = " << ptr << std::endl;
		return ptr;
	}

	void* mm_alloc(size_t size_in_bytes)
	{
		//std::cout << "ALLOC(" << size_in_bytes << ")" << std::endl;
		mm_block_t* bptr = nullptr;
		if ((bptr = find_free_block(size_in_bytes + MM_BLOCK_OVERHEAD)) != nullptr) {
			//std::cout << "found free block = " << (void*)bptr << std::endl;
			block_markused(bptr);
			/* split if we have a block too large for us? */
			split_block(bptr, size_in_bytes);
		} else {
			//std::cout << "no free block found that is big enough!" << std::endl;
			// check if last block is free
			//std::cout << "check last block" << std::endl;
			bptr = last_block();
			if (bptr && block_isfree(bptr)) {
				//std::cout << "last block is free. -> extend!" << std::endl;
				// extent last block as it is free
				size_t blockdatasize = block_getdatasize(bptr);
				size_t needed		 = ALIGN(size_in_bytes - blockdatasize);
				hsbrk(needed);
				remove_from_free_set(bptr);
				block_update(bptr,
							 blockdatasize + needed + sizeof(size_t) + sizeof(mm_block_foot_t));
				//insert_into_free_set(bptr);
				block_markused(bptr);
			} else {
				bptr = new_block(size_in_bytes);
			}
		}
		return block_data(bptr);
	}

	void mm_free(void* ptr)
	{
		if (ptr) {
			mm_block_t* bptr = block_cur(ptr);
			block_markfree(bptr);
			/* coalesce if needed. otherwise just add */
			coalesce_block(bptr);
		}
	}


	bool in_address_space(void* ptr)
	{
		// check if ptr is in the hugepage address space
		if (ptr == nullptr) {
			return true;
		}
		if (ptr >= m_base && ptr < m_top) {
			return true;
		}
		return false;
	}
};
#endif // end MSVC_COMPILER

} // end namespace

#endif
