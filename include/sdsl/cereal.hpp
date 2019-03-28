// Copyright (c) 2018, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file cereal.hpp
    \brief cereal.hpp offers cereal support
*/

#ifndef INCLUDED_SDSL_CEREAL
#define INCLUDED_SDSL_CEREAL

#define CEREAL_SERIALIZE_FUNCTION_NAME cereal_serialize
#define CEREAL_LOAD_FUNCTION_NAME cereal_load
#define CEREAL_SAVE_FUNCTION_NAME cereal_save
#define CEREAL_LOAD_MINIMAL_FUNCTION_NAME cereal_load_minimal
#define CEREAL_SAVE_MINIMAL_FUNCTION_NAME cereal_save_minimal

#if defined(__has_include)
	#if __has_include(<cereal/cereal.hpp>)
		#define SDSL_HAS_CEREAL 1
		#include <cereal/cereal.hpp>
		#include <cereal/details/traits.hpp>
		#include <cereal/archives/binary.hpp>
		#include <cereal/archives/json.hpp>
		#include <cereal/archives/portable_binary.hpp>
		#include <cereal/archives/xml.hpp>
		#include <cereal/types/memory.hpp>
		#include <cereal/types/vector.hpp>
	#endif
#endif

#ifndef SDSL_HAS_CEREAL
	#define SDSL_HAS_CEREAL 0

	#define CEREAL_NVP(X) X

	namespace cereal
	{
		namespace traits
		{
			template <typename t1, typename t2>
			struct is_output_serializable
			{
				using value = std::false_type;
			};

			template <typename t1, typename t2>
			struct is_input_serializable
			{
				using value = std::false_type;
			};
		}

	template <typename t>
	struct BinaryData {};

	template <typename t1, typename t2>
	void make_nvp(t1 const &, t2 const &) {}

	template <typename t>
	void make_size_tag(t const &) {}

	template <typename t1, typename t2>
	t1 binary_data(t1 const &, t2 const &) {}

	}
#endif
#endif
