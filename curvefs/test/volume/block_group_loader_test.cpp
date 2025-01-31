/*
 *  Copyright (c) 2022 NetEase Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 * Project: curve
 * Date: Monday Mar 07 10:14:21 CST 2022
 * Author: wuhanqing
 */

#include "curvefs/src/volume/block_group_loader.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/memory/memory.h"
#include "curvefs/test/volume/common.h"
#include "curvefs/test/volume/mock/mock_block_device_client.h"

namespace curvefs {
namespace volume {

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

// TODO(wuhanqing): random choose values
static constexpr uint32_t kBlockSize = 4 * kKiB;
static constexpr uint64_t kBlockGroupSize = 128 * kMiB;   // 128 MiB
static constexpr uint64_t kBlockGroupOffset = 10 * kGiB;  // 10 GiB
static constexpr BitmapLocation kBitmapLocation = BitmapLocation::AtStart;

class BlockGroupBitmapLoaderTest : public ::testing::Test {
 protected:
    void SetUp() override {
        option_.type = "bitmap";
        option_.bitmapAllocatorOption.sizePerBit = 4 * kMiB;
        option_.bitmapAllocatorOption.smallAllocProportion = 0;

        loader_ = absl::make_unique<BlockGroupBitmapLoader>(
            &blockDev_, kBlockSize, kBlockGroupOffset, kBlockGroupSize,
            kBitmapLocation, option_);
    }

 protected:
    MockBlockDeviceClient blockDev_;
    AllocatorOption option_;
    std::unique_ptr<BlockGroupBitmapLoader> loader_;
};

TEST_F(BlockGroupBitmapLoaderTest, LoadTest_ReadBitmapError) {
    EXPECT_CALL(blockDev_, Read(_, _, _))
        .WillOnce(Return(false));

    AllocatorAndBitmapUpdater dummy;
    ASSERT_FALSE(loader_->Load(&dummy));
}

TEST_F(BlockGroupBitmapLoaderTest, LoadTest_CreataAllocatorError) {
    option_.type = "none";

    EXPECT_CALL(blockDev_, Read(_, _, _))
        .WillOnce(Return(true));

    AllocatorAndBitmapUpdater dummy;
    ASSERT_FALSE(loader_->Load(&dummy));
}

TEST_F(BlockGroupBitmapLoaderTest, LoadTest_Success) {
    EXPECT_CALL(blockDev_, Read(_, _, _))
        .WillOnce(Invoke([](char* buf, off_t /*offset*/, size_t length) {
            memset(buf, 0, length);
            return true;
        }));

    AllocatorAndBitmapUpdater dummy;
    ASSERT_TRUE(loader_->Load(&dummy));

    ASSERT_EQ(kBlockGroupSize - kBlockGroupSize / kBlockSize / 8,
              dummy.allocator->AvailableSize());
    ASSERT_EQ(kBlockGroupSize, dummy.allocator->Total());
}

}  // namespace volume
}  // namespace curvefs
