// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#ifndef INCLUDED_SDSL_CONSTRUCT_LCP_HELPER
#define INCLUDED_SDSL_CONSTRUCT_LCP_HELPER

#include "sdsl/int_vector.hpp"
#include <queue>
#include <list>
#include <vector>

namespace sdsl {

//! Merges a partial LCP array into the LCP array on disk.
/*!
 * \param partial_lcp		Vector containing LCP values for all indexes \f$i\f$ with
 *                      	index_done[i] == 0. Let x=partail_lcp[rank(index_done, i, 0)];
 *                      	LCP[i]=x if x!=0 and index_done[i] == 0
 * \param lcp_file			Path to the LCP array on disk.
 * \param index_done		Entry index_done[i] indicates if LCP[i] is already calculated.
 * \param max_lcp_value 	Maximum known LCP value
 * \param lcp_value_offset	Largest LCP value in lcp_file
 */
inline void insert_lcp_values(int_vector<>& partial_lcp,
					   bit_vector&   index_done,
					   std::string   lcp_file,
					   uint64_t		 max_lcp_value,
					   uint64_t		 lcp_value_offset)
{
	std::string						tmp_lcp_file = lcp_file + "_TMP";
	const uint64_t					buffer_size  = 1000000; // has to be a multiple of 64
	typedef int_vector<>::size_type size_type;
	int_vector_buffer<> lcp_buffer(lcp_file, std::ios::in, buffer_size); // open lcp_file
	uint64_t			n = lcp_buffer.size();

    // open tmp_lcp_file
    uint8_t int_width = bits::hi(max_lcp_value)+1;
    int_vector_buffer<> out_buf(tmp_lcp_file, std::ios::out, buffer_size, int_width);		// Output buffer
    // Write values into buffer
    for (size_type i=0, calc_idx=0; i < n; ++i) {
        if (index_done[i]) {   // If value was already calculated
            out_buf[i] = lcp_buffer[i]; // Copy value
        } else {
            if (partial_lcp[calc_idx]) {   // If value was calculated now
                // Insert value
                out_buf[i] = partial_lcp[calc_idx]+lcp_value_offset;
                index_done[i] = true;
            }
            ++calc_idx;
        }
    }
    lcp_buffer.close();
    out_buf.close();
    // Close file and replace old file with new one
    sdsl::rename(tmp_lcp_file, lcp_file);
}


template <class tWT>
void create_C_array(std::vector<uint64_t>& C, const tWT& wt)
{
	uint64_t				   quantity;	  // quantity of characters in interval
	std::vector<unsigned char> cs(wt.sigma);  // list of characters in the interval
	std::vector<uint64_t> rank_c_i(wt.sigma); // number of occurrence of character in [0 .. i-1]
	std::vector<uint64_t> rank_c_j(wt.sigma); // number of occurrence of character in [0 .. j-1]

	C = std::vector<uint64_t>(257, 0);
	interval_symbols(wt, 0, wt.size(), quantity, cs, rank_c_i, rank_c_j);
	for (uint64_t i = 0; i < quantity; ++i) {
		unsigned char c = cs[i];
		C[c + 1]		= rank_c_j[i];
	}
	for (uint64_t i = 1; i < C.size() - 1; ++i) {
		C[i + 1] += C[i];
	}
}


class buffered_char_queue {
	typedef bit_vector::size_type size_type;
	typedef std::queue<uint8_t>   tQ;

private:
	static const uint32_t m_buffer_size = 10000; //409600;
	uint8_t				  m_write_buf[m_buffer_size];
	uint8_t				  m_read_buf[m_buffer_size];
	size_type			  m_widx;	 // write index
	size_type			  m_ridx;	 // read index
	bool				  m_sync;	 // are read and write buffer the same?
	size_type m_disk_buffered_blocks; // number of blocks written to disk and not read again yet
	char	  m_c;
	size_type m_rb; // read blocks
	size_type m_wb; // written blocks

	std::string m_file_name;

	std::fstream m_stream;

public:
    buffered_char_queue()
        : m_widx(0), m_ridx(0), m_sync(true), m_disk_buffered_blocks(0), m_c('?'), m_rb(0), m_wb(0)
    { }
    
	void init(const std::string& dir, char c)
    {
        m_c			= c;
        m_file_name = dir + "buffered_char_queue_" + util::to_string(util::pid());
        //		m_stream.rdbuf()->pubsetbuf(0, 0);
    }
    
    ~buffered_char_queue()
    {
        m_stream.close();
        sdsl::remove(m_file_name);
    }
    
	void push_back(uint8_t x)
    {
        m_write_buf[m_widx] = x;
        if (m_sync) {
            m_read_buf[m_widx] = x;
        }
        ++m_widx;
        if (m_widx == m_buffer_size) {
            if (!m_sync) { // if not sync, write block to disk
                if (!m_stream.is_open()) {
                    m_stream.open(m_file_name,
                                  std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
                }
                m_stream.seekp(m_buffer_size * (m_wb++), std::ios::beg);
                m_stream.write((char*)m_write_buf, m_buffer_size);
                ++m_disk_buffered_blocks;
            }
            m_sync = 0;
            m_widx = 0;
        }
    }

	uint8_t pop_front()
    {
        uint8_t x = m_read_buf[m_ridx];
        ++m_ridx;
        if (m_ridx == m_buffer_size) {
            if (m_disk_buffered_blocks > 0) {
                m_stream.seekg(m_buffer_size * (m_rb++), std::ios::beg);
                m_stream.read((char*)m_read_buf, m_buffer_size);
                --m_disk_buffered_blocks;
            } else { // m_disk_buffered_blocks == 0
                m_sync = 1;
                memcpy(m_read_buf, m_write_buf, m_widx + 1);
            }
            m_ridx = 0;
        }
        return x;
    }

};

typedef std::list<int_vector<>::size_type>   tLI;
typedef std::vector<int_vector<>::size_type> tVI;

template <class size_type_class>
void push_front_m_index(size_type_class i,
						uint8_t			c,
						tLI (&m_list)[256],
						uint8_t (&m_chars)[256],
						size_type_class& m_char_count)
{
	if (m_list[c].empty()) {
		m_chars[m_char_count++] = c;
	}
	m_list[c].push_front(i);
}

template <class size_type_class>
void push_back_m_index(size_type_class i,
					   uint8_t		   c,
					   tLI (&m_list)[256],
					   uint8_t (&m_chars)[256],
					   size_type_class& m_char_count)
{
	if (m_list[c].empty()) {
		m_chars[m_char_count++] = c;
	}
	m_list[c].push_back(i);
}

}

#endif
