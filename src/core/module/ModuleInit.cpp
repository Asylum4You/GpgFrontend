/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "ModuleInit.h"

#include <QCoreApplication>
#include <QDir>

#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"

namespace GpgFrontend::Module {

void LoadModuleFromPath(const QString& mods_path, bool integrated) {
  for (const auto& module_library_name :
       QDir(mods_path).entryList(QStringList() << "*.so"
                                               << "*.dll"
                                               << "*.dylib",
                                 QDir::Files)) {
    ModuleManager::GetInstance().LoadModule(
        mods_path + "/" + module_library_name, integrated);
  }
}

auto LoadIntegratedMods() -> bool {
  auto exec_binary_path = QCoreApplication::applicationDirPath();

#if defined(MACOS) && defined(RELEASE)
  // App Bundle
  auto mods_path = exec_binary_path + "/../PlugIns/mods";
#else
  // Debug Or Windows Platform
  auto mods_path = exec_binary_path + "/mods";
#endif

  // AppImage
  if (!qEnvironmentVariable("APPIMAGE").isEmpty()) {
    mods_path = qEnvironmentVariable("APPDIR") + "/usr/plugins/mods";
  }

  // Flatpak
  if (!qEnvironmentVariable("container").isEmpty()) {
    mods_path = "/app/lib/mods";
  }

  GF_CORE_LOG_DEBUG("try loading integrated modules at path: {} ...",
                    mods_path);
  if (!QDir(mods_path).exists()) {
    GF_CORE_LOG_WARN(
        "integrated module directory at path {} not found, abort...",
        mods_path);
    return false;
  }

  LoadModuleFromPath(mods_path, true);

  GF_CORE_LOG_DEBUG("load integrated modules done.");
  return true;
}

auto LoadExternalMods() -> bool {
  auto mods_path =
      GpgFrontend::GlobalSettingStation::GetInstance().GetModulesDir();

  GF_CORE_LOG_DEBUG("try loading external modules at path: {} ...", mods_path);
  if (!QDir(mods_path).exists()) {
    GF_CORE_LOG_WARN("external module directory at path {} not found, abort...",
                     mods_path);
    return false;
  }

  LoadModuleFromPath(mods_path, false);

  GF_CORE_LOG_DEBUG("load integrated modules done.");
  return true;
}

void LoadGpgFrontendModules(ModuleInitArgs) {
  // must init at default thread before core
  Thread::TaskRunnerGetter::GetInstance().GetTaskRunner()->PostTask(
      new Thread::Task(
          [](const DataObjectPtr&) -> int {
            return LoadIntegratedMods() && LoadExternalMods() ? 0 : -1;
          },
          "modules_system_init_task"));
}

void ShutdownGpgFrontendModules() {}

}  // namespace GpgFrontend::Module