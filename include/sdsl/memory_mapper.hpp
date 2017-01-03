// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#ifndef INCLUDED_SDSL_MEMORY_MAPPER
#define INCLUDED_SDSL_MEMORY_MAPPER

#include "memory_tracking.hpp"
#include "ram_fs.hpp"

namespace sdsl {

struct memory_mapper {
	static int open_file_for_mmap(std::string& filename, std::ios_base::openmode mode)
	{
		if (is_ram_file(filename)) {
			return ram_fs::open(filename);
		}
#ifdef MSVC_COMPILER
		int fd = -1;
		if (!(mode & std::ios_base::out))
			_sopen_s(&fd, filename.c_str(), _O_BINARY | _O_RDONLY, _SH_DENYNO, _S_IREAD);
		else
			_sopen_s(&fd, filename.c_str(), _O_BINARY | _O_RDWR, _SH_DENYNO, _S_IREAD | _S_IWRITE);
		return fd;
#else
		if (!(mode & std::ios_base::out))
			return open(filename.c_str(), O_RDONLY);
		else
			return open(filename.c_str(), O_RDWR);
#endif
		return -1;
	}

	static void* mmap_file(int fd, uint64_t file_size, std::ios_base::openmode mode)
	{
		if (file_size == 0) {
			std::cout << "file_size=0" << std::endl;
			return nullptr;
		}
		if (is_ram_file(fd)) {
			if (ram_fs::file_size(fd) < file_size) return nullptr;
			auto& file_content = ram_fs::content(fd);
			return file_content.data();
		}
		memory_monitor::record(file_size);
#ifdef MSVC_COMPILER
		HANDLE fh = (HANDLE)_get_osfhandle(fd);
		if (fh == INVALID_HANDLE_VALUE) {
			return nullptr;
		}
		HANDLE fm;
		if (!(mode & std::ios_base::out)) { // read only?
			fm = CreateFileMapping(fh, NULL, PAGE_READONLY, 0, 0, NULL);
		} else
			fm = CreateFileMapping(fh, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (fm == NULL) {
			return nullptr;
		}
		void* map = nullptr;
		if (!(mode & std::ios_base::out)) { // read only?
			map = MapViewOfFile(fm, FILE_MAP_READ, 0, 0, file_size);
		} else
			map = MapViewOfFile(fm, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, file_size);
		// we can close the file handle before we unmap the view: (see UnmapViewOfFile Doc)
		// Although an application may close the file handle used to create a file mapping object,
		// the system holds the corresponding file open until the last view of the file is unmapped.
		// Files for which the last view has not yet been unmapped are held open with no sharing restrictions.
		CloseHandle(fm);
		return map;
#else
		void* map = nullptr;
		if (!(mode & std::ios_base::out))
			map = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
		else
			map = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (map == MAP_FAILED) map = nullptr; // unify windows and unix error behaviour
		return map;
#endif
		return nullptr;
	}

	static int mem_unmap(int fd, void* addr, const uint64_t size)
	{
		if (addr == nullptr) {
			return 0;
		}
		if (is_ram_file(fd)) {
			return 0;
		}
		memory_monitor::record(-((int64_t)size));
#ifdef MSVC_COMPILER
		if (UnmapViewOfFile(addr)) return 0;
		return -1;
#else
		return munmap(addr, size);
#endif
		return -1;
	}

	static int close_file_for_mmap(int fd)
	{
		if (is_ram_file(fd)) {
			return ram_fs::close(fd);
		}
#ifdef MSVC_COMPILER
		return _close(fd);
#else
		return close(fd);
#endif
		return -1;
	}

	static int truncate_file_mmap(int fd, const uint64_t new_size)
	{
		if (is_ram_file(fd)) {
			return ram_fs::truncate(fd, new_size);
		}
#ifdef MSVC_COMPILER
		auto ret		  = _chsize_s(fd, new_size);
		if (ret != 0) ret = -1;
		return ret;
#else
		return ftruncate(fd, new_size);
#endif
		return -1;
	}
};

} // end namespace

#endif
