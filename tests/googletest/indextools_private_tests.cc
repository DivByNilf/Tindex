#include <gtest/gtest.h>

#include "indextools_private.hpp"

// Demonstrate some basic assertions.
TEST(IndexToolsPrivateTest, IdMapTests) {
	IndexID indexID_1 = MainIndexIndex::indexID;

	std::map<IndexID, uint64_t> idMap;

	uint64_t uint1 = 44;
	uint64_t uint2 = 45;
	uint64_t uint3 = 46;

	ASSERT_EQ(idMap.size(), 0) << "map size was not 0";

	auto resPair = idMap.emplace(indexID_1, uint1);

	ASSERT_NE(resPair.first, idMap.end());

	EXPECT_EQ(resPair.second, true);

	ASSERT_EQ(idMap.size(), 1);

	//

	EXPECT_EQ(std::less<IndexID>()(indexID_1, indexID_1), false);

	// it should be a iterator to the pre-existing entry instead
	//EXPECT_NE(resPair.first, ID_Map.end());

	//EXPECT_EQ(resPair.second, false);

	//EXPECT_EQ(ID_Map.size(), 1);

	//

	auto resID_PairIt = idMap.find(indexID_1);

	ASSERT_NE(resID_PairIt, idMap.end());

	EXPECT_EQ(resID_PairIt->second, uint1);

	// a second id

	IndexID indexID_2 = DirIndex::indexID;

	resID_PairIt = idMap.find(indexID_2);

	EXPECT_EQ(resID_PairIt, idMap.end());

	resPair = idMap.emplace(indexID_2, uint2);

	ASSERT_EQ(idMap.size(), 2);

	//

	resID_PairIt = idMap.find(indexID_2);

	ASSERT_NE(resID_PairIt, idMap.end());

	EXPECT_EQ(resID_PairIt->second, uint2);

	//

	resID_PairIt = idMap.find(indexID_2);

	ASSERT_NE(resID_PairIt, idMap.end());

	EXPECT_EQ(resID_PairIt->second, uint2);

	// a third id (with a blank string)
	// there's no rule against a blank string currently

	IndexID indexID_3 = IndexID("");

	resID_PairIt = idMap.find(indexID_3);

	EXPECT_EQ(resID_PairIt, idMap.end());

	resPair = idMap.emplace(indexID_3, uint3);

	ASSERT_EQ(idMap.size(), 3);

	//

	resID_PairIt = idMap.find(indexID_3);

	ASSERT_NE(resID_PairIt, idMap.end());

	EXPECT_EQ(resID_PairIt->second, uint3);

}
