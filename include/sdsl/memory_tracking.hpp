// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file memory_tracking.hpp
\brief memory_tracking.hpp contains two function for allocating and deallocating memory
\author Simon Gog
*/
#ifndef INCLUDED_SDSL_MEMORY_TRACKING
#define INCLUDED_SDSL_MEMORY_TRACKING

#include "uintx_t.hpp"
#include "config.hpp"
#include "bits.hpp"
//#include "ram_fs.hpp"

#include <map>
#include <iostream>
#include <cstdlib>
#include <mutex>
#include <chrono>
#include <cstring>
#include <set>
#include <cstddef>
#include <stack>
#include <vector>
#include <atomic>
#include "config.hpp"
#include <fcntl.h>
#include <sstream>
#include <fstream>

#ifdef _WIN32
#ifndef NOMINMAX
// windows.h has min/max macro which causes problems when using std::min/max
#define NOMINMAX 1
#endif
#include <windows.h>
#include <io.h>
#else
#include <sys/mman.h>
#include <unistd.h> // for getpid, file_size, clock_gettime
#endif


namespace sdsl {

//forward declare    
void memory_monitor_record(int64_t);

// minimal allocator from http://stackoverflow.com/a/21083096
template <typename T>
struct track_allocator {
  using value_type = T;

  track_allocator() = default;
  template <class U>
  track_allocator(const track_allocator<U>&) {}

  T* allocate(std::size_t n) {
    if (n <= std::numeric_limits<std::size_t>::max() / sizeof(T)) {
      size_t s = n * sizeof(T);
      if (auto ptr = std::malloc(s)) {
        memory_monitor_record(s);
        return static_cast<T*>(ptr);
      }
    }
    throw std::bad_alloc();
  }
  void deallocate(T* ptr, std::size_t n) {
    std::free(ptr);
    std::size_t s = n * sizeof(T);
    memory_monitor_record(-((int64_t)s));
  }
};

template <typename T, typename U>
inline bool operator == (const track_allocator<T>&, const track_allocator<U>&) {
  return true;
}

template <typename T, typename U>
inline bool operator != (const track_allocator<T>& a, const track_allocator<U>& b) {
  return !(a == b);
}



// spin lock
class spin_lock
{
    private:
        std::atomic_flag m_slock;
    public:
        spin_lock()
        {
            m_slock.clear();
        }
        void lock()
        {
            while (m_slock.test_and_set(std::memory_order_acquire)) {
                /* spin */
            }
        };
        void unlock()
        {
            m_slock.clear(std::memory_order_release);
        };
};

namespace ram_fs{
    typedef std::vector<char, track_allocator<char>> content_type;
}



struct ramfs_storage {
    typedef std::map<std::string, ram_fs::content_type> mss_type;
    typedef std::map<int, std::string> mis_type;
    std::recursive_mutex m_rlock;

    mss_type m_map;
    mis_type m_fd_map;

    ramfs_storage(){
        m_fd_map[-1] = "";
    }

    ~ramfs_storage(){ }
};

struct mm_alloc {
    using timer = std::chrono::high_resolution_clock;
    timer::time_point timestamp;
    int64_t usage;
    mm_alloc(timer::time_point t, int64_t u) : timestamp(t), usage(u) {};
};

struct mm_event {
    using timer = std::chrono::high_resolution_clock;
    std::string name;
    std::vector<mm_alloc> allocations;
    mm_event(std::string n, int64_t usage) : name(n)
    {
        allocations.emplace_back(timer::now(), usage);
    };
    bool operator< (const mm_event& a) const
    {
        if (a.allocations.size() && this->allocations.size()) {
            if (this->allocations[0].timestamp == a.allocations[0].timestamp) {
                return this->allocations.back().timestamp < a.allocations.back().timestamp;
            } else {
                return this->allocations[0].timestamp < a.allocations[0].timestamp;
            }
        }
        return true;
    }
};

struct tracker_storage {
        using timer = std::chrono::high_resolution_clock;
        std::chrono::milliseconds log_granularity = std::chrono::milliseconds(20ULL);
        int64_t current_usage = 0;
        bool track_usage = false;
        std::vector<mm_event> completed_events;
        std::stack<mm_event> event_stack;
        timer::time_point start_log;
        timer::time_point last_event;
        spin_lock spinlock;

        tracker_storage(){ }

        ~tracker_storage(){ }
};

template<format_type F>
void write_mem_log(std::ostream& out, const tracker_storage& m);



class memory_monitor
{
    public:
        using timer = std::chrono::high_resolution_clock;

        struct mm_event_proxy {
            bool add;
            timer::time_point created;
            mm_event_proxy(const std::string& name, int64_t usage, bool a) : add(a)
            {
                if (add) {
                    auto& m = *(the_monitor().m_tracker);
                    std::lock_guard<spin_lock> lock(m.spinlock);
                    m.event_stack.emplace(name, usage);
                }
            }
            ~mm_event_proxy()
            {
                if (add) {
                    auto& m = *(the_monitor().m_tracker);
                    std::lock_guard<spin_lock> lock(m.spinlock);
                    auto& cur = m.event_stack.top();
                    auto cur_time = timer::now();
                    cur.allocations.emplace_back(cur_time, m.current_usage);
                    m.completed_events.emplace_back(std::move(cur));
                    m.event_stack.pop();
                    // add a point to the new "top" with the same memory
                    // as before but just ahead in time
                    if (!m.event_stack.empty()) {
                        if (m.event_stack.top().allocations.size()) {
                            auto last_usage = m.event_stack.top().allocations.back().usage;
                            m.event_stack.top().allocations.emplace_back(cur_time, last_usage);
                        }
                    }
                }
            }
        };


    private:
        tracker_storage* m_tracker;
        ramfs_storage* m_ram_fs;

        // disable construction of the object
        memory_monitor() {
            m_tracker = new tracker_storage();
            m_ram_fs = new ramfs_storage();

        };

        ~memory_monitor()
        {
            if (m_tracker->track_usage) {
                stop();
            }
            delete m_ram_fs;
            delete m_tracker;
        }
        memory_monitor(const memory_monitor&) = delete;
        memory_monitor& operator=(const memory_monitor&) = delete;

        static memory_monitor& the_monitor()
        {
            static memory_monitor m;
            return m;
        }
    public:

        static void granularity(std::chrono::milliseconds ms)
        {
            auto& m = *(the_monitor().m_tracker);
            m.log_granularity = ms;
        }
        static int64_t peak()
        {
            auto& m = *(the_monitor().m_tracker);
            int64_t max = 0;
            for (auto events : m.completed_events) {
                for (auto alloc : events.allocations) {
                    if (max < alloc.usage) {
                        max = alloc.usage;
                    }
                }
            }
            return max;
        }

        static ramfs_storage& ram_fs(){
            return *(the_monitor().m_ram_fs);
        }

        static void start()
        {
            auto& m = *(the_monitor().m_tracker);
            m.track_usage = true;
            // clear if there is something there
            if (m.completed_events.size()) {
                m.completed_events.clear();
            }
            while (m.event_stack.size()) {
                m.event_stack.pop();
            }
            m.start_log = timer::now();
            m.current_usage = 0;
            m.last_event = m.start_log;
            m.event_stack.emplace("unknown", 0);
        }
        static void stop()
        {
            auto& m = *(the_monitor().m_tracker);
            while (!m.event_stack.empty()) {
                m.completed_events.emplace_back(std::move(m.event_stack.top()));
                m.event_stack.pop();
            }
            m.track_usage = false;
        }
        static void record(int64_t delta)
        {
            auto& m = *(the_monitor().m_tracker);
            if (m.track_usage) {
                std::lock_guard<spin_lock> lock(m.spinlock);
                auto cur = timer::now();
                if (m.last_event + m.log_granularity < cur) {
                    m.event_stack.top().allocations.emplace_back(cur, m.current_usage);
                    m.current_usage = m.current_usage + delta;
                    m.event_stack.top().allocations.emplace_back(cur, m.current_usage);
                    m.last_event = cur;
                } else {
                    if (m.event_stack.top().allocations.size()) {
                        m.current_usage = m.current_usage + delta;
                        m.event_stack.top().allocations.back().usage = m.current_usage;
                        m.event_stack.top().allocations.back().timestamp = cur;
                    }
                }
            }
        }

        static mm_event_proxy event(const std::string& name)
        {
            auto& m = *(the_monitor().m_tracker);
            if (m.track_usage) {
                return mm_event_proxy(name, m.current_usage, true);
            }
            return mm_event_proxy(name, m.current_usage, false);
        }

        template<format_type F>
        static void write_memory_log(std::ostream& out)
        {
            write_mem_log<F>(out, *(the_monitor().m_tracker));
        }
};

inline void memory_monitor_record(int64_t delta) {
    memory_monitor::record(delta);
}

} // end namespace

#endif
