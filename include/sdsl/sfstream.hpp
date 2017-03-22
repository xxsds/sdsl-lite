// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file sfstream.hpp
   \brief sfstream.hpp contains a two stream class which can be used to read/write from/to files or strings.
   \author Simon Gog
*/
#ifndef INCLUDED_SDSL_SFSTREAM
#define INCLUDED_SDSL_SFSTREAM

#include <fstream>
#include <sstream>
#include <string>
#include "sdsl/ram_fs.hpp"
#include "sdsl/ram_filebuf.hpp"

namespace sdsl {

class osfstream : public std::ostream {
public:
	typedef std::streambuf* buf_ptr_type;

private:
	buf_ptr_type m_streambuf = nullptr;
	std::string  m_file		 = "";

public:
	typedef void* voidptr;
	//! Standard constructor.
    osfstream() : std::ostream(nullptr) { this->init(m_streambuf); }

	//! Constructor taking a file name and open mode.
    osfstream(const std::string& file, std::ios_base::openmode mode = std::ios_base::out) : std::ostream(nullptr)
    {
        this->init(m_streambuf);
        open(file, mode);
    }

	//! Open the stream.
	buf_ptr_type open(const std::string& file, std::ios_base::openmode mode = std::ios_base::out)
    {
        delete m_streambuf;
        m_streambuf				= nullptr;
        m_file					= file;
        std::streambuf* success = nullptr;
        if (is_ram_file(file)) {
            m_streambuf = new ram_filebuf();
            success		= ((ram_filebuf*)m_streambuf)->open(m_file, mode | std::ios_base::out);
        } else {
            m_streambuf = new std::filebuf();
            success		= ((std::filebuf*)m_streambuf)->open(m_file, mode | std::ios_base::out);
        }
        if (success) {
            this->clear();
        } else {
            this->setstate(std::ios_base::failbit);
            delete m_streambuf;
            m_streambuf = nullptr;
        }
        this->rdbuf(m_streambuf);
        return m_streambuf;
    }

	//! Is the stream close?
	bool is_open()
    {
        if (nullptr == m_streambuf) return false;
        if (is_ram_file(m_file)) {
            return ((ram_filebuf*)m_streambuf)->is_open();
        } else {
            return ((std::filebuf*)m_streambuf)->is_open();
        }
    }

	//! Close the stream.
	void close()
    {
        bool fail = false;
        if (nullptr == m_streambuf) {
            fail = true;
        } else {
            if (is_ram_file(m_file)) {
                fail = !((ram_filebuf*)m_streambuf)->close();
            } else {
                fail = !((std::filebuf*)m_streambuf)->close();
            }
        }
        if (fail) this->setstate(std::ios::failbit);
    }

	//! Standard destructor
	~osfstream()
    {
        delete m_streambuf; // streambuf closes the file on destruction
    }

	//! Cast to void*
    operator voidptr() const { return m_streambuf; }

	osfstream& seekp(pos_type pos)
    {
        ios_base::iostate err = std::ios_base::iostate(std::ios_base::goodbit);
        try {
            if (!this->fail()) {
                pos_type p = 0;
                if (is_ram_file(m_file)) {
                    p = ((ram_filebuf*)m_streambuf)->pubseekpos(pos, std::ios_base::out);
                } else {
                    p = ((std::filebuf*)m_streambuf)->pubseekpos(pos, std::ios_base::out);
                }
                if (p == pos_type(off_type(-1))) {
                    err |= ios_base::failbit;
                    this->setstate(err);
                }
            }
        } catch (...) {
            if (err) {
                this->setstate(err);
            }
        }
        return *this;
    }

	osfstream& seekp(off_type off, ios_base::seekdir way)
    {
        ios_base::iostate err = std::ios_base::iostate(ios_base::goodbit);
        try {
            if (!this->fail()) {
                pos_type p = 0;
                if (is_ram_file(m_file)) {
                    p = ((ram_filebuf*)m_streambuf)->pubseekoff(off, way, std::ios_base::out);

                } else {
                    p = ((std::filebuf*)m_streambuf)->pubseekoff(off, way, std::ios_base::out);
                }
                if (p == pos_type(off_type(-1))) {
                    err |= ios_base::failbit;
                    this->setstate(err);
                }
            }
        } catch (...) {
            if (err) {
                this->setstate(err);
            }
        }
        return *this;
    }

	std::streampos tellp();
};


class isfstream : public std::istream {
	typedef std::streambuf* buf_ptr_type;

private:
	buf_ptr_type m_streambuf = nullptr;
	std::string  m_file		 = "";

public:
	typedef void* voidptr;
	//! Standard constructor.
    isfstream() : std::istream(nullptr) { this->init(m_streambuf); }

	//! Constructor taking a file name and open mode.
	isfstream(const std::string& file, std::ios_base::openmode mode = std::ios_base::in)
     : std::istream(nullptr)
    {
        this->init(m_streambuf);
        open(file, mode);
    }

	//! Open the stream.
	buf_ptr_type open(const std::string& file, std::ios_base::openmode mode = std::ios_base::in)
    {
        delete m_streambuf;
        m_streambuf				= nullptr;
        m_file					= file;
        std::streambuf* success = nullptr;
        if (is_ram_file(file)) {
            m_streambuf = new ram_filebuf();
            success		= ((ram_filebuf*)m_streambuf)->open(m_file, mode | std::ios_base::in);
        } else {
            m_streambuf = new std::filebuf();
            success		= ((std::filebuf*)m_streambuf)->open(m_file, mode | std::ios_base::in);
        }
        if (success) {
            this->clear();
        } else {
            this->setstate(std::ios_base::failbit);
            delete m_streambuf;
            m_streambuf = nullptr;
        }
        this->rdbuf(m_streambuf);
        return m_streambuf;
    }

	//! Is the stream close?
	bool is_open()
    {
        if (nullptr == m_streambuf) return false;
        if (is_ram_file(m_file)) {
            return ((ram_filebuf*)m_streambuf)->is_open();
        } else {
            return ((std::filebuf*)m_streambuf)->is_open();
        }
    }
    
	//! Close the stream.
	void close()
    {
        bool fail = false;
        if (nullptr == m_streambuf) {
            fail = true;
        } else {
            if (is_ram_file(m_file)) {
                fail = !((ram_filebuf*)m_streambuf)->close();
            } else {
                fail = !((std::filebuf*)m_streambuf)->close();
            }
        }
        if (fail) this->setstate(std::ios::failbit);
    }
    
	//! Standard destructor
    ~isfstream() { delete m_streambuf; }

	//! Cast to void*
    operator voidptr() const
    {
        return m_streambuf; // streambuf closes the file on destruction
    }

	isfstream& seekg(pos_type pos)
    {
        ios_base::iostate err = std::ios_base::iostate(std::ios_base::goodbit);
        try {
            if (!this->fail()) {
                pos_type p = 0;
                if (is_ram_file(m_file)) {
                    p = ((ram_filebuf*)m_streambuf)->pubseekpos(pos, std::ios_base::in);

                } else {
                    p = ((std::filebuf*)m_streambuf)->pubseekpos(pos, std::ios_base::in);
                }
                if (p == pos_type(off_type(-1))) {
                    err |= ios_base::failbit;
                }
            }
        } catch (...) {
            if (err) {
                this->setstate(err);
            }
        }
        return *this;
    }

	isfstream& seekg(off_type off, ios_base::seekdir way) 
    {
        ios_base::iostate err = std::ios_base::iostate(ios_base::goodbit);
        try {
            if (!this->fail()) {
                pos_type p = 0;
                if (is_ram_file(m_file)) {
                    p = ((ram_filebuf*)m_streambuf)->pubseekoff(off, way, std::ios_base::in);

                } else {
                    p = ((std::filebuf*)m_streambuf)->pubseekoff(off, way, std::ios_base::in);
                }
                if (p == pos_type(off_type(-1))) {
                    err |= ios_base::failbit;
                }
            }
        } catch (...) {
            if (err) {
                this->setstate(err);
            }
        }
        return *this;
    }

	std::streampos tellg()
    {
        ios_base::iostate err = std::ios_base::iostate(ios_base::goodbit);
        pos_type		  p   = pos_type(off_type(-1));
        try {
            if (!this->fail()) {
                if (is_ram_file(m_file)) {
                    p = ((ram_filebuf*)m_streambuf)->pubseekoff(0, std::ios_base::cur);

                } else {
                    p = ((std::filebuf*)m_streambuf)->pubseekoff(0, std::ios_base::cur);
                }
                if (p == pos_type(off_type(-1))) {
                    err |= ios_base::failbit;
                }
            }
        } catch (...) {
            if (err) {
                this->setstate(err);
            }
        }
        return p;
    }
};

} // end namespace

#endif
