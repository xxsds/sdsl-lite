#include <cstdlib>
#include <vector>

#include <sdsl/ram_fs.hpp>

namespace
{

// This container is constructed before the monitor is first used in main().
std::vector<int, sdsl::track_allocator<int>> values;

void check_shutdown()
{
    // This callback was also registered before the monitor's first use.
    // Both allocation and RAM-file access must still work during shutdown.
    values.push_back(43);
    if (values.size() != 2 || values[0] != 42 || values[1] != 43
        || sdsl::ram_fs::content("@shutdown") != sdsl::ram_fs::content_type{'o', 'k'})
    {
        std::abort();
    }
    sdsl::ram_fs::remove("@shutdown");
    sdsl::memory_monitor::stop();
}

} // namespace

int main()
{
    if (std::atexit(check_shutdown) != 0)
        return EXIT_FAILURE;

    sdsl::memory_monitor::start();
    values.push_back(42);
    sdsl::ram_fs::store("@shutdown", {'o', 'k'});
}
