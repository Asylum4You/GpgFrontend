/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <chrono>
#include <thread>

#include "GpgCoreTest.h"
#include "core/GpgConstants.h"
#include "core/function/CacheManager.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreCacheTestA) {
  CacheManager::GetInstance().SaveCache("ABC", "DEF");
  ASSERT_EQ(CacheManager::GetInstance().LoadCache("ABC"), QString("DEF"));
  ASSERT_EQ(CacheManager::GetInstance().LoadCache("ABCGG"), QString());
}

TEST_F(GpgCoreTest, CoreCacheTestB) {
  CacheManager::GetInstance().SaveCache("ABCDE", "DEFG", 3);
  ASSERT_EQ(CacheManager::GetInstance().LoadCache("ABCDE"), QString("DEFG"));
}

TEST_F(GpgCoreTest, CoreCacheTestC) {
  CacheManager::GetInstance().SaveCache("ABCDEF", "DEF", 2);
  ASSERT_EQ(CacheManager::GetInstance().LoadCache("ABCDEF"), QString("DEFG"));
  std::this_thread::sleep_for(std::chrono::milliseconds(4000));
  ASSERT_EQ(CacheManager::GetInstance().LoadCache("ABCDEF"), QString(""));
}

}  // namespace GpgFrontend::Test