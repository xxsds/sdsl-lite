#ifndef DOC_LIST_INDEX
#define DOC_LIST_INDEX

#include <string>

#include <sdsl/construct.hpp>

struct doc_list_tag
{};

template <class t_index>
void construct(t_index & idx, std::string const & file, sdsl::cache_config & config, uint8_t num_bytes, doc_list_tag)
{
    idx = t_index(file, config, num_bytes);
}

#include "doc_list_index_greedy.hpp"
#include "doc_list_index_qprobing.hpp"
#include "doc_list_index_sada.hpp"
#include "doc_list_index_sort.hpp"

#endif
