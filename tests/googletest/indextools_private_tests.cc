#include <gtest/gtest.h>

#include "indextools_private.hpp"

// Demonstrate some basic assertions.
TEST(IndexToolsPrivateTest, IdMapTests) {
	IndexID index_id_1 = MainIndexIndex::indexID;

	std::map<IndexID, uint64_t> id_map;

	uint64_t uint_1 = 44;
	uint64_t uint_2 = 45;
	uint64_t uint_3 = 46;

	ASSERT_EQ(id_map.size(), 0) << "map size was not 0";

	auto res_pair = id_map.emplace(index_id_1, uint_1);

	ASSERT_NE(res_pair.first, id_map.end());

	EXPECT_EQ(res_pair.second, true);

	ASSERT_EQ(id_map.size(), 1);

	//

	EXPECT_EQ(std::less<IndexID>()(index_id_1, index_id_1), false);

	// it should be a iterator to the pre-existing entry instead
	//EXPECT_NE(res_pair.first, id_map.end());

	//EXPECT_EQ(res_pair.second, false);

	//EXPECT_EQ(id_map.size(), 1);

	//

	auto res_id_pair_it = id_map.find(index_id_1);

	ASSERT_NE(res_id_pair_it, id_map.end());

	EXPECT_EQ(res_id_pair_it->second, uint_1);

	// a second id

	IndexID index_id_2 = DirIndex::indexID;

	res_id_pair_it = id_map.find(index_id_2);

	EXPECT_EQ(res_id_pair_it, id_map.end());

	res_pair = id_map.emplace(index_id_2, uint_2);

	ASSERT_EQ(id_map.size(), 2);

	//

	res_id_pair_it = id_map.find(index_id_2);

	ASSERT_NE(res_id_pair_it, id_map.end());

	EXPECT_EQ(res_id_pair_it->second, uint_2);

	//

	res_id_pair_it = id_map.find(index_id_2);

	ASSERT_NE(res_id_pair_it, id_map.end());

	EXPECT_EQ(res_id_pair_it->second, uint_2);

	// a third id (with a blank string)
	// there's no rule against a blank string currently

	IndexID index_id_3 = IndexID("");

	res_id_pair_it = id_map.find(index_id_3);

	EXPECT_EQ(res_id_pair_it, id_map.end());

	res_pair = id_map.emplace(index_id_3, uint_3);

	ASSERT_EQ(id_map.size(), 3);

	//

	res_id_pair_it = id_map.find(index_id_3);

	ASSERT_NE(res_id_pair_it, id_map.end());

	EXPECT_EQ(res_id_pair_it->second, uint_3);

}
