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

#include "Log.h"

#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "core/function/GlobalSettingStation.h"

namespace GpgFrontend::Module::SDK {

void InitModuleLoggingSystem(spdlog::level::level_enum level) {
  // get the log directory
  auto log_file_path =
      (GpgFrontend::GlobalSettingStation::GetInstance().GetLogDir() / "module");
  log_file_path.replace_extension(".log");

  // sinks
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(GpgFrontend::SecureCreateSharedObject<
                  spdlog::sinks::stderr_color_sink_mt>());
  sinks.push_back(GpgFrontend::SecureCreateSharedObject<
                  spdlog::sinks::rotating_file_sink_mt>(
      log_file_path.u8string(), 1048576 * 32, 32));

  // thread pool
  spdlog::init_thread_pool(1024, 2);

  // logger
  auto module_logger =
      GpgFrontend::SecureCreateSharedObject<spdlog::async_logger>(
          "module", begin(sinks), end(sinks), spdlog::thread_pool());
  module_logger->set_pattern(
      "[%H:%M:%S.%e] [T:%t] [%=6n] %^[%=8l]%$ [%s:%#] [%!] -> %v (+%ius)");

  // set the level of logger
  module_logger->set_level(level);

  // flush policy
  module_logger->flush_on(spdlog::level::err);
  spdlog::flush_every(std::chrono::seconds(5));

  // register it as default logger
  spdlog::set_default_logger(module_logger);
}

void ShutdownModuleLoggingSystem() {
#ifdef WINDOWS
  // Under VisualStudio, this must be called before main finishes to workaround
  // a known VS issue
  spdlog::drop_all();
  spdlog::shutdown();
#endif
}

std::shared_ptr<spdlog::logger> GetModuleLogger() {
  return spdlog::get("module");
}

}  // namespace GpgFrontend::Module::SDK
