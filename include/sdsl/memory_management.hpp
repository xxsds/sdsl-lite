// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#ifndef INCLUDED_SDSL_MEMORY_MANAGEMENT
#define INCLUDED_SDSL_MEMORY_MANAGEMENT

#include "uintx_t.hpp"
#include "config.hpp"
#include "bits.hpp"
#include "memory_tracking.hpp"
#include "memory_hugepages.hpp"

namespace sdsl {

class memory_manager {
private:
	bool hugepages = false;
#ifndef MSVC_COMPILER
	hugepage_allocator hp_allocator;
#endif
private:
	static memory_manager& the_manager()
	{
		static memory_manager m;
		return m;
	}

public:
	static uint64_t* alloc_mem(size_t size_in_bytes)
	{
#ifndef MSVC_COMPILER
		auto& m = the_manager();
		if (m.hugepages) {
			return (uint64_t*)m.hp_allocator.mm_alloc(size_in_bytes);
		}
#endif
		return (uint64_t*)calloc(size_in_bytes, 1);
	}
	static void free_mem(uint64_t* ptr)
	{
#ifndef MSVC_COMPILER
		auto& m = the_manager();
		if (m.hugepages and m.hp_allocator.in_address_space(ptr)) {
			m.hp_allocator.mm_free(ptr);
			return;
		}
#endif
		std::free(ptr);
	}
	static uint64_t* realloc_mem(uint64_t* ptr, size_t size)
	{
#ifndef MSVC_COMPILER
		auto& m = the_manager();
		if (m.hugepages and m.hp_allocator.in_address_space(ptr)) {
			return (uint64_t*)m.hp_allocator.mm_realloc(ptr, size);
		}
#endif
		return (uint64_t*)realloc(ptr, size);
	}

public:
	static void use_hugepages(size_t bytes = 0)
	{
#ifndef MSVC_COMPILER
		auto& m = the_manager();
		m.hp_allocator.init(bytes);
		m.hugepages = true;
#else
		throw std::runtime_error("hugepages not support on MSVC_COMPILER");
#endif
	}
	template <class t_vec>
	static void resize(t_vec& v, const typename t_vec::size_type size)
	{
		uint64_t old_size_in_bytes = ((v.m_size + 63) >> 6) << 3;
		uint64_t new_size_in_bytes = ((size + 63) >> 6) << 3;
		bool	 do_realloc		   = old_size_in_bytes != new_size_in_bytes;
		v.m_size				   = size;
		if (do_realloc || v.m_data == nullptr) {
			// Note that we allocate 8 additional bytes if m_size % 64 == 0.
			// We need this padding since rank data structures do a memory
			// access to this padding to answer rank(size()) if size()%64 ==0.
			// Note that this padding is not counted in the serialize method!
			size_t allocated_bytes = (size_t)(((size + 64) >> 6) << 3);
			v.m_data			   = memory_manager::realloc_mem(v.m_data, allocated_bytes);
			if (allocated_bytes != 0 && v.m_data == nullptr) {
				throw std::bad_alloc();
			}
			// update and fill with 0s
			if (v.bit_size() < v.capacity()) {
				uint8_t len			   = (uint8_t)(v.capacity() - v.bit_size());
				uint8_t in_word_offset = (uint8_t)(v.bit_size() & 0x3F);
				bits::write_int(v.m_data + (v.bit_size() >> 6), 0, in_word_offset, len);
			}
			if (((v.m_size) % 64) == 0) { // initialize unreachable bits with 0
				v.m_data[v.m_size / 64] = 0;
			}

			// update stats
			if (do_realloc) {
				memory_monitor::record((int64_t)new_size_in_bytes - (int64_t)old_size_in_bytes);
			}
		}
	}
	template <class t_vec>
	static void clear(t_vec& v)
	{
		int64_t size_in_bytes = ((v.m_size + 63) >> 6) << 3;
		// remove mem
		memory_manager::free_mem(v.m_data);
		v.m_data = nullptr;

		// update stats
		if (size_in_bytes) {
			memory_monitor::record(size_in_bytes * -1);
		}
	}
};

} // end namespace

#endif
