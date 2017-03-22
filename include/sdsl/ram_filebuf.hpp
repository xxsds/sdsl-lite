// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#ifndef INCLUDED_SDSL_RAM_FSTREAMBUF
#define INCLUDED_SDSL_RAM_FSTREAMBUF

#include <fstream>
#include <vector>
#include "ram_fs.hpp"

namespace sdsl {

class ram_filebuf : public std::streambuf
{
    private:
        // TODO:  also store filename/descriptor to implement is_open ???
        ram_fs::content_type* m_ram_file = nullptr;  // file handle
        void pbump64(std::ptrdiff_t x) 
        {
            while (x > std::numeric_limits<int>::max()) {
                pbump(std::numeric_limits<int>::max());
                x -= std::numeric_limits<int>::max();
            }
            pbump(x);
        }


public:
	virtual ~ram_filebuf() {};

	ram_filebuf() {};

    ram_filebuf(ram_fs::content_type& ram_file) : m_ram_file(&ram_file)
    {
        char* begin = m_ram_file->data();
        char* end   = begin + m_ram_file->size();
        setg(begin, begin, end); // set get pointers eback(), eptr(), egptr()
    }

    std::streambuf* open(const std::string name, std::ios_base::openmode mode)
    {
        // open ram_file
        if ((mode & std::ios_base::in) and !(mode & std::ios_base::trunc)) {
            // file must exist, initial position at the start
            if (!ram_fs::exists(name)) {
                m_ram_file = nullptr;
            } else {
                m_ram_file = &ram_fs::content(name);
            }
        } else { // existence of file not required
            if (!ram_fs::exists(name)) {
                // create empty file, if it does not yet exist
                ram_fs::store(
                name, ram_fs::content_type()); // TODO: create method in ram_fs?? or store w 1 arg?
            }
            m_ram_file = &ram_fs::content(name);
            if ((mode & std::ios_base::out) and !(mode & std::ios_base::app)) {
                m_ram_file->clear();
            }
        }

        if (m_ram_file and (mode & std::ios_base::trunc)) {
            m_ram_file->clear();
        }
        if (m_ram_file) {
            if (mode & std::ios_base::ate) {
                // TODO: move put pointer to the end of the file
            } else {
            }
            setg(m_ram_file->data(), m_ram_file->data(), m_ram_file->data() + m_ram_file->size());
            setp(m_ram_file->data(), m_ram_file->data() + m_ram_file->size());
        }
        // ATTENTION: if m_ram_file->size() == 0, then data might be nullptr !!!
        return m_ram_file ? this : nullptr;
    }



    bool is_open() { return m_ram_file != nullptr; }

	ram_filebuf* close()
    {
        if (!this->is_open()) return nullptr;
        m_ram_file = nullptr;
        setg(nullptr, nullptr, nullptr);
        setp(nullptr, nullptr);
        return this;
    }



	pos_type seekpos(pos_type				 sp,
					 std::ios_base::openmode mode = std::ios_base::in |
												    std::ios_base::out) override
    {
        if (sp >= (pos_type)0 and sp <= (pos_type)m_ram_file->size()) {
            setg(m_ram_file->data(), m_ram_file->data() + sp, m_ram_file->data() + m_ram_file->size());
            setp(m_ram_file->data(), m_ram_file->data() + m_ram_file->size());
            pbump64(sp);
        } else {
            if (mode & std::ios_base::out) {
                // extend buffer
                m_ram_file->reserve(sp);
                m_ram_file->resize(sp, 0);
                setg(
                m_ram_file->data(), m_ram_file->data() + sp, m_ram_file->data() + m_ram_file->size());
                setp(m_ram_file->data(), m_ram_file->data() + m_ram_file->size());
                pbump64(sp);
            } else {
                return pos_type(off_type(-1));
            }
        }
        return sp;
    }


	pos_type pubseekoff(off_type				off,
						std::ios_base::seekdir  way,
						std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
        if (std::ios_base::beg == way) {
            if (seekpos(off, which) == pos_type(-1)) {
                return pos_type(-1);
            }
        } else if (std::ios_base::cur == way) {
            if (seekpos(gptr() - eback() + off, which) == pos_type(-1)) {
                return pos_type(-1);
            }
        } else if (std::ios_base::end == way) {
            if (seekpos(egptr() - eback() + off, which) == pos_type(-1)) {
                return pos_type(-1);
            }
        }
        return gptr() - eback();
    }

	pos_type pubseekpos(pos_type				sp,
						std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
        if (seekpos(sp, which) == pos_type(-1)) {
            return pos_type(-1);
        } else {
            return gptr() - eback();
        }
    }

	std::streamsize xsputn(const char_type* s, std::streamsize n) override
    {
        //    std::cout<<"xsputn( , of size "<<n<<")"<<std::endl;
        //    std::cout<<"epptr()-pptr()="<<epptr()-pptr()<<std::endl;
        if (!m_ram_file) {
           return 0;
        }

        if (n < epptr() - pptr()) {
           std::copy(s, s + n, pptr());
           pbump64(n);
           return n;
        } else {
           if (epptr() - pbase() == (std::ptrdiff_t)m_ram_file->size() and epptr() == pptr()) {
               m_ram_file->insert(m_ram_file->end(), s, s + n);
               setp(m_ram_file->data(), m_ram_file->data() + m_ram_file->size());
               std::ptrdiff_t add = epptr() - pbase();
               pbump64(add);
               setg(m_ram_file->data(), gptr(), m_ram_file->data() + m_ram_file->size());
               return n;
           } else {
               for (std::streamsize i = 0; i < n; ++i) {
                   if (traits_type::eq_int_type(sputc(s[i]), traits_type::eof())) {
                       return i;
                   }
               }
               return n;
           }
        }
    }       

	int sync() override 
    {
	    return 0; // we are always in sync, since buffer is sink
    }

	int_type overflow(int_type c = traits_type::eof()) override
    {
        if (m_ram_file) {
            m_ram_file->push_back(c);
            setp(m_ram_file->data(), m_ram_file->data() + m_ram_file->size());
            std::ptrdiff_t add = epptr() - pbase();
            pbump64(add);
            setg(m_ram_file->data(), gptr(), m_ram_file->data() + m_ram_file->size());
        }
        return traits_type::to_int_type(c);
    }
};
}

#endif
