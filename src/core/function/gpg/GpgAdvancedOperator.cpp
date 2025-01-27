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

//
// Created by eric on 07.01.2023.
//

#include "GpgAdvancedOperator.h"

#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/module/ModuleManager.h"
#include "core/utils/GpgUtils.h"

void GpgFrontend::GpgAdvancedOperator::ClearGpgPasswordCache(
    OperationCallback cb) {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  if (gpgconf_path.isEmpty()) {
    FLOG_W("cannot get valid gpgconf path from rt, abort.");
    if (cb) cb(-1, TransferParams());
    return;
  }

  auto key_dbs = GetGpgKeyDatabaseInfos();
  auto total_tasks = static_cast<int>(key_dbs.size());
  std::atomic<int> completed_tasks{0};
  std::vector<int> results(total_tasks, 0);

  int task_index = 0;
  for (const auto &key_db : key_dbs) {
    const int current_index = task_index++;
    const auto target_home_dir =
        QDir::toNativeSeparators(QFileInfo(key_db.path).canonicalFilePath());

    GpgFrontend::GpgCommandExecutor::ExecuteSync(
        {gpgconf_path,
         QStringList{"--homedir", target_home_dir, "--reload", "gpg-agent"},
         [=, &completed_tasks, &results](int exit_code, const QString &,
                                         const QString &) {
           FLOG_D("gpgconf reload exit code: %d", exit_code);

           results[current_index] = exit_code;

           if (++completed_tasks == total_tasks && cb) {
             int final_result =
                 std::all_of(results.begin(), results.end(),
                             [](int result) { return result >= 0; })
                     ? 0
                     : -1;
             cb(final_result, TransferParams());
           }
         }});
  }
}

void GpgFrontend::GpgAdvancedOperator::ReloadGpgComponents(
    OperationCallback cb) {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  if (gpgconf_path.isEmpty()) {
    FLOG_W("cannot get valid gpgconf path from rt, abort.");
    if (cb) cb(-1, TransferParams());
    return;
  }

  auto key_dbs = GetGpgKeyDatabaseInfos();
  auto total_tasks = static_cast<int>(key_dbs.size());
  std::atomic<int> completed_tasks{0};
  std::vector<int> results(total_tasks, 0);

  int task_index = 0;
  for (const auto &key_db : key_dbs) {
    const int current_index = task_index++;
    const auto target_home_dir =
        QDir::toNativeSeparators(QFileInfo(key_db.path).canonicalFilePath());

    GpgFrontend::GpgCommandExecutor::ExecuteSync(
        {gpgconf_path,
         QStringList{"--homedir", target_home_dir, "--reload", "all"},
         [=, &completed_tasks, &results](int exit_code, const QString &,
                                         const QString &) {
           FLOG_D("gpgconf reload exit code: %d", exit_code);
           results[current_index] = exit_code;

           if (++completed_tasks == total_tasks && cb) {
             int final_result =
                 std::all_of(results.begin(), results.end(),
                             [](int result) { return result >= 0; })
                     ? 0
                     : -1;
             cb(final_result, TransferParams());
           }
         }});
  }
}

void GpgFrontend::GpgAdvancedOperator::KillAllGpgComponents(
    OperationCallback cb) {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  if (gpgconf_path.isEmpty()) {
    FLOG_W("cannot get valid gpgconf path from rt, abort.");
    if (cb) cb(-1, TransferParams());
    return;
  }

  auto key_dbs = GetGpgKeyDatabaseInfos();
  auto total_tasks = static_cast<int>(key_dbs.size());
  std::atomic<int> completed_tasks{0};
  std::vector<int> results(total_tasks, 0);

  int task_index = 0;
  for (const auto &key_db : key_dbs) {
    const int current_index = task_index++;
    const auto target_home_dir =
        QDir::toNativeSeparators(QFileInfo(key_db.path).canonicalFilePath());

    LOG_D() << "closing all gpg component at home path: " << target_home_dir;
    GpgFrontend::GpgCommandExecutor::ExecuteSync(
        {gpgconf_path,
         QStringList{"--homedir", target_home_dir, "--kill", "all"},
         [=, &completed_tasks, &results](int exit_code, const QString &p_out,
                                         const QString &p_err) {
           FLOG_D("gpgconf --kill --all exit code: %d", exit_code);

           results[current_index] = exit_code;

           if (++completed_tasks == total_tasks && cb) {
             int final_result =
                 std::all_of(results.begin(), results.end(),
                             [](int result) { return result >= 0; })
                     ? 0
                     : -1;
             cb(final_result, TransferParams());
           }
         }});
  }
}

void GpgFrontend::GpgAdvancedOperator::RestartGpgComponents(
    OperationCallback cb) {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  if (gpgconf_path.isEmpty()) {
    FLOG_W("cannot get valid gpgconf path from rt, abort.");
    if (cb) cb(-1, TransferParams());
    return;
  }

  KillAllGpgComponents(nullptr);

  LaunchGpgComponents(cb);
}

void GpgFrontend::GpgAdvancedOperator::ResetConfigures(OperationCallback cb) {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  if (gpgconf_path.isEmpty()) {
    FLOG_W("cannot get valid gpgconf path from rt, abort.");
    if (cb) cb(-1, TransferParams());
    return;
  }

  auto key_dbs = GetGpgKeyDatabaseInfos();
  auto total_tasks = static_cast<int>(key_dbs.size());
  std::atomic<int> completed_tasks{0};
  std::vector<int> results(total_tasks, 0);

  int task_index = 0;
  for (const auto &key_db : key_dbs) {
    const int current_index = task_index++;
    const auto target_home_dir =
        QDir::toNativeSeparators(QFileInfo(key_db.path).canonicalFilePath());

    GpgFrontend::GpgCommandExecutor::ExecuteSync(
        {gpgconf_path,
         QStringList{"--homedir", target_home_dir, "--apply-defaults"},
         [=, &completed_tasks, &results](int exit_code, const QString &,
                                         const QString &) {
           FLOG_D("gpgconf --apply-defaults exit code: %d", exit_code);

           results[current_index] = exit_code;

           if (++completed_tasks == total_tasks && cb) {
             int final_result =
                 std::all_of(results.begin(), results.end(),
                             [](int result) { return result >= 0; })
                     ? 0
                     : -1;
             cb(final_result, TransferParams());
           }
         }});
  }
}

void GpgFrontend::GpgAdvancedOperator::LaunchGpgComponents(
    OperationCallback cb) {
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  if (gpgconf_path.isEmpty()) {
    FLOG_W("cannot get valid gpgconf path from rt, abort.");
    if (cb) cb(-1, TransferParams());
    return;
  }

  auto key_dbs = GetGpgKeyDatabaseInfos();
  auto total_tasks = static_cast<int>(key_dbs.size());
  std::atomic<int> completed_tasks{0};
  std::vector<int> results(total_tasks, 0);

  int task_index = 0;
  for (const auto &key_db : key_dbs) {
    const int current_index = task_index++;
    const auto target_home_dir =
        QDir::toNativeSeparators(QFileInfo(key_db.path).canonicalFilePath());

    GpgFrontend::GpgCommandExecutor::ExecuteSync(
        {gpgconf_path,
         QStringList{"--homedir", target_home_dir, "--launch", "all"},
         [=, &completed_tasks, &results](int exit_code, const QString &,
                                         const QString &) {
           FLOG_D("gpgconf --launch all exit code: %d", exit_code);

           results[current_index] = exit_code;

           if (++completed_tasks == total_tasks && cb) {
             int final_result =
                 std::all_of(results.begin(), results.end(),
                             [](int result) { return result >= 0; })
                     ? 0
                     : -1;
             cb(final_result, TransferParams());
           }
         }});
  }
}
