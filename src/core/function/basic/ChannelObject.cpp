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

#include "ChannelObject.h"

#include <iostream>

namespace GpgFrontend {

ChannelObject::ChannelObject() noexcept = default;

ChannelObject::ChannelObject(int channel, QString type)
    : channel_(channel), type_(std::move(type)) {}

#ifdef DEBUG
ChannelObject::~ChannelObject() noexcept {
  LOG_D() << "Releasing Channel Object: " << this->type_;
}
#else
ChannelObject::~ChannelObject() noexcept = default;
#endif

void ChannelObject::SetChannel(int channel) { this->channel_ = channel; }

auto ChannelObject::GetChannel() const -> int { return channel_; }

auto ChannelObject::GetDefaultChannel() -> int {
  return kGpgFrontendDefaultChannel;
}

}  // namespace GpgFrontend