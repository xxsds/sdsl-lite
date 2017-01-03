// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file ram_fs.hpp
 * \brief ram_fs.hpp
 * \author Simon Gog
 */
#ifndef INCLUDED_SDSL_RAM_FS
#define INCLUDED_SDSL_RAM_FS

#include "uintx_t.hpp"
#include "memory_tracking.hpp"
#include <string>
#include <map>
#include <vector>
#include <mutex>

namespace sdsl {

class ram_fs;

//! ram_fs is a simple store for RAM-files.
/*!
 * (strings) to file content (content_type).
 */
class ram_fs {
public:
	typedef std::vector<char, track_allocator<char>> content_type;

private:
	typedef std::map<std::string, content_type> mss_type;
	typedef std::map<int, std::string>			mis_type;
	mss_type			 m_map;
	std::recursive_mutex m_rlock;
	mis_type			 m_fd_map;

	static ram_fs& the_ramfs()
	{
		static ram_fs fs;
		return fs;
	}

public:
	//! Default construct
	ram_fs() { m_fd_map[-1] = ""; }

	//! store file to ram file system
	static void store(const std::string& name, content_type data)
	{
		auto& r = ram_fs::the_ramfs();

		std::lock_guard<std::recursive_mutex> lock(r.m_rlock);
		if (!exists(name)) {
			std::string cname = name;
			r.m_map.insert(std::make_pair(std::move(cname), std::move(data)));
		} else {
			r.m_map[name] = std::move(data);
		}
	}

	//! Check if the file exists
	static bool exists(const std::string& name)
	{
		auto& r = ram_fs::the_ramfs();

		std::lock_guard<std::recursive_mutex> lock(r.m_rlock);
		return r.m_map.find(name) != r.m_map.end();
	}

	//! Get the file size
	static size_t file_size(const std::string& name)
	{
		auto& r = ram_fs::the_ramfs();

		std::lock_guard<std::recursive_mutex> lock(r.m_rlock);
		if (exists(name)) {
			return r.m_map[name].size();
		} else {
			return 0;
		}
	}

	//! Get the content
	static content_type& content(const std::string& name)
	{
		auto& r = ram_fs::the_ramfs();

		std::lock_guard<std::recursive_mutex> lock(r.m_rlock);
		return r.m_map[name];
	}


	//! Remove the file with key `name`
	static int remove(const std::string& name)
	{
		auto& r = ram_fs::the_ramfs();

		std::lock_guard<std::recursive_mutex> lock(r.m_rlock);
		r.m_map.erase(name);
		return 0;
	}

	//! Rename the file. Change key `old_filename` into `new_filename`.
	static int rename(const std::string old_filename, const std::string new_filename)
	{
		auto& r = ram_fs::the_ramfs();

		std::lock_guard<std::recursive_mutex> lock(r.m_rlock);
		r.m_map[new_filename] = std::move(r.m_map[old_filename]);
		remove(old_filename);
		return 0;
	}

	//! Get fd for file
	static int open(const std::string& name)
	{
		auto& r = ram_fs::the_ramfs();

		std::lock_guard<std::recursive_mutex> lock(r.m_rlock);
		if (!exists(name)) {
			store(name, content_type{});
		}
		int  fd			= -2;
		auto largest_fd = r.m_fd_map.rbegin()->first;
		if (largest_fd < 0) {
			auto smallest_fd = r.m_fd_map.begin()->first;
			fd				 = smallest_fd - 1;
		} else {
			r.m_fd_map.erase(largest_fd);
			fd = -largest_fd;
		}
		r.m_fd_map[fd] = name;
		return fd;
	}

	//! Get fd for file
	static int close(const int fd)
	{
		auto& r = ram_fs::the_ramfs();

		std::lock_guard<std::recursive_mutex> lock(r.m_rlock);
		if (fd >= -1) return -1;
		if (r.m_fd_map.count(fd) == 0) {
			return -1;
		} else {
			r.m_fd_map.erase(fd);
			r.m_fd_map[-fd] = "";
		}
		return 0;
	}

	//! Get the content with fd
	static content_type& content(const int fd)
	{
		auto& r = ram_fs::the_ramfs();

		std::lock_guard<std::recursive_mutex> lock(r.m_rlock);
		auto								  name = r.m_fd_map[fd];
		return r.m_map[name];
	}
	//! Get the content with fd
	static int truncate(const int fd, size_t new_size)
	{
		auto& r = ram_fs::the_ramfs();

		std::lock_guard<std::recursive_mutex> lock(r.m_rlock);
		if (r.m_fd_map.count(fd) == 0) return -1;
		auto name = r.m_fd_map[fd];
		r.m_map[name].reserve(new_size);
		r.m_map[name].resize(new_size, 0);
		return 0;
	}


	//! Get the file size with fd_
	static size_t file_size(const int fd)
	{
		auto& r = ram_fs::the_ramfs();

		std::lock_guard<std::recursive_mutex> lock(r.m_rlock);
		if (r.m_fd_map.count(fd) == 0) return 0;
		auto name = r.m_fd_map[fd];
		return r.m_map[name].size();
	}
};

//! Determines if the given file is a RAM-file.
inline bool is_ram_file(const std::string& file)
{
	if (file.size() > 0) {
		if (file[0] == '@') {
			return true;
		}
	}
	return false;
}

//! Determines if the given file is a RAM-file.
inline bool is_ram_file(const int fd) { return fd < -1; }

//! Returns the corresponding RAM-file name for file.
inline std::string ram_file_name(const std::string& file)
{
	if (is_ram_file(file)) {
		return file;
	} else {
		return "@" + file;
	}
}

//! Returns for a RAM-file the corresponding disk file name
inline std::string disk_file_name(const std::string& file)
{
	if (!is_ram_file(file)) {
		return file;
	} else {
		return file.substr(1);
	}
}

//! Remove a file.
inline int remove(const std::string& file)
{
	if (is_ram_file(file)) {
		return ram_fs::remove(file);
	} else {
		return std::remove(file.c_str());
	}
}

//! Rename a file
inline int rename(const std::string& old_filename, const std::string& new_filename)
{
	if (is_ram_file(old_filename)) {
		if (!is_ram_file(new_filename)) { // error, if new file is not also RAM-file
			return -1;
		}
		return ram_fs::rename(old_filename, new_filename);
	} else {
		return std::rename(old_filename.c_str(), new_filename.c_str());
	}
}


} // end namespace sdsl
#endif
