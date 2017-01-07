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

namespace ram_fs{

        //! Check if the file exists
        inline bool exists(const std::string& name){
            auto& rf = memory_monitor::ram_fs();
            std::lock_guard<std::recursive_mutex> lock(rf.m_rlock);
            return rf.m_map.find(name) != rf.m_map.end();
        }

        inline void store(const std::string& name, content_type data) {
            auto& rf = memory_monitor::ram_fs();
            std::lock_guard<std::recursive_mutex> lock(rf.m_rlock);
            if (!exists(name)) { 
                std::string cname = name;
                rf.m_map.insert(std::make_pair(std::move(cname), std::move(data)));
            } else {
                rf.m_map[name] = std::move(data);
            }
        }
        

        //! Get the file size
        inline size_t file_size(const std::string& name) {
            auto& rf = memory_monitor::ram_fs();
            std::lock_guard<std::recursive_mutex> lock(rf.m_rlock);
            if (exists(name)) {
                return rf.m_map[name].size();
            } else {
                return 0;
            }
        }

        //! Get the content
        inline content_type& content(const std::string& name) {
            auto& rf = memory_monitor::ram_fs();
            std::lock_guard<std::recursive_mutex> lock(rf.m_rlock);
            return rf.m_map[name];
        }

        //! Remove the file with key `name`
        inline int remove(const std::string& name) {
            auto& rf = memory_monitor::ram_fs();
            std::lock_guard<std::recursive_mutex> lock(rf.m_rlock);
            if (exists(name)) {
                rf.m_map.erase(name);
            }
            return 0;
        }

        //! Rename the file. Change key `old_filename` into `new_filename`.
        inline int rename(const std::string old_filename, const std::string new_filename){
            auto& rf = memory_monitor::ram_fs();
            std::lock_guard<std::recursive_mutex> lock(rf.m_rlock);
            rf.m_map[new_filename] = std::move(rf.m_map[old_filename]);
            remove(old_filename);
            return 0;
        }

        //! Get fd for file
        inline int open(const std::string& name) {
            auto& rf = memory_monitor::ram_fs();
            std::lock_guard<std::recursive_mutex> lock(rf.m_rlock);
            if(!exists(name)) {
                store(name,content_type{});
            }
            int fd = -2;
            auto largest_fd = rf.m_fd_map.rbegin()->first;
            if( largest_fd < 0 ) {
                auto smallest_fd = rf.m_fd_map.begin()->first;
                fd = smallest_fd - 1;
            } else {
                rf.m_fd_map.erase(largest_fd);
                fd = - largest_fd;
            }
            rf.m_fd_map[fd] = name;
            return fd;
        }

        //! Get fd for file
        inline int close(const int fd) {
            auto& rf = memory_monitor::ram_fs();
            std::lock_guard<std::recursive_mutex> lock(rf.m_rlock);
            if( fd >= -1 ) return -1;
            if(rf.m_fd_map.count(fd) == 0) {
                return -1;
            } else {
                rf.m_fd_map.erase(fd);
                rf.m_fd_map[-fd] = "";
            }
            return 0;
        }

        //! Get the content with fd
        inline content_type& content(const int fd){
            auto& rf = memory_monitor::ram_fs();
            std::lock_guard<std::recursive_mutex> lock(rf.m_rlock);
            auto name = rf.m_fd_map[fd];
            return rf.m_map[name];
        }

        //! Get the content with fd
        inline int truncate(const int fd,size_t new_size) {
            auto& rf = memory_monitor::ram_fs();
            std::lock_guard<std::recursive_mutex> lock(rf.m_rlock);
            if(rf.m_fd_map.count(fd) == 0) return -1;
            auto name = rf.m_fd_map[fd];
            rf.m_map[name].reserve(new_size);
            rf.m_map[name].resize(new_size,0);
            return 0;
        }

        //! Get the file size with fd
        inline size_t file_size(const int fd) {
            auto& rf = memory_monitor::ram_fs();
            std::lock_guard<std::recursive_mutex> lock(rf.m_rlock);
            if(rf.m_fd_map.count(fd) == 0) return 0;
            auto name = rf.m_fd_map[fd];
            return rf.m_map[name].size();
        }

}// end namespace ram_fs

//! Determines if the given file is a RAM-file.
inline bool is_ram_file(const std::string& file) {
    if (file.size() > 0) {
        if (file[0]=='@') {
            return true;
        }
    }
    return false;
}

//! Determines if the given file is a RAM-file.
inline bool is_ram_file(const int fd) {
    return fd < -1;
}

//! Returns the corresponding RAM-file name for file.
inline std::string ram_file_name(const std::string& file) {
    if (is_ram_file(file)) {
        return file;
    } else {
        return "@" + file;
    }
}

//! Returns for a RAM-file the corresponding disk file name
inline std::string disk_file_name(const std::string& file) {
    if (!is_ram_file(file)) {
        return file;
    } else {
        return file.substr(1);
    }
}


//! Remove a file.
inline int remove(const std::string& file) {
    if (is_ram_file(file)) {
        return ram_fs::remove(file);
    } else {
        return std::remove(file.c_str());
    }

}

//! Rename a file
inline int rename(const std::string& old_filename, const std::string& new_filename){
    if (is_ram_file(old_filename)) {
        if (!is_ram_file(new_filename)) {  // error, if new file is not also RAM-file
            return -1;
        }
        return ram_fs::rename(old_filename, new_filename);
    } else {
        return std::rename(old_filename.c_str(), new_filename.c_str());
    }
}




} // end namespace sdsl
#endif
