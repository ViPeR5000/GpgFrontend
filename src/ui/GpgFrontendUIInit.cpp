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

#include "GpgFrontendUIInit.h"

#include <qapplication.h>
#include <qcoreapplication.h>
#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <QtNetwork>
#include <string>

#include "core/GpgConstants.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/module/ModuleManager.h"
#include "core/thread/TaskRunnerGetter.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/gnupg/GnuPGControllerDialog.h"
#include "ui/main_window/MainWindow.h"

#if !defined(RELEASE) && defined(WINDOWS)
#include "core/function/GlobalSettingStation.h"
#endif

namespace GpgFrontend::UI {

extern void InitLocale();

void WaitEnvCheckingProcess() {
  SPDLOG_DEBUG("need to waiting for env checking process");

  // create and show loading window before starting the main window
  auto* waiting_dialog = new QProgressDialog();
  waiting_dialog->setMaximum(0);
  waiting_dialog->setMinimum(0);
  auto* waiting_dialog_label =
      new QLabel(QString(_("Loading Gnupg Info...")) + "<br /><br />" +
                 _("If this process is too slow, please set the key "
                   "server address appropriately in the gnupg configuration "
                   "file (depending "
                   "on the network situation in your country or region)."));
  waiting_dialog_label->setWordWrap(true);
  waiting_dialog->setLabel(waiting_dialog_label);
  waiting_dialog->resize(420, 120);
  QApplication::connect(CoreSignalStation::GetInstance(),
                        &CoreSignalStation::SignalGoodGnupgEnv, waiting_dialog,
                        [=]() {
                          SPDLOG_DEBUG("gpg env loaded successfuly");
                          waiting_dialog->finished(0);
                          waiting_dialog->deleteLater();
                        });

  // new local event looper
  QEventLoop looper;
  QApplication::connect(CoreSignalStation::GetInstance(),
                        &CoreSignalStation::SignalGoodGnupgEnv, &looper,
                        &QEventLoop::quit);

  QApplication::connect(waiting_dialog, &QProgressDialog::canceled, [=]() {
    SPDLOG_DEBUG("cancel clicked on wairing dialog");
    QApplication::quit();
    exit(0);
  });

  auto env_state = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "env.state.basic", std::string{"0"});

  SPDLOG_DEBUG("ui is ready to wating for env initialized, env_state: {}",
               env_state);

  // check twice to avoid some unlucky sitations
  if (env_state == "1") {
    SPDLOG_DEBUG("env state turned initialized before the looper start");
    waiting_dialog->finished(0);
    waiting_dialog->deleteLater();
    return;
  }

  // show the loading window
  waiting_dialog->setModal(true);
  waiting_dialog->setFocus();
  waiting_dialog->show();

  // block the main thread until the gpg context is loaded
  looper.exec();
}

void PreInitGpgFrontendUI() { CommonUtils::GetInstance(); }

void InitGpgFrontendUI(QApplication* /*app*/) {
  // init locale
  InitLocale();

#if !defined(RELEASE) && defined(WINDOWS)
  // css
  std::filesystem::path css_path =
      GpgFrontend::GlobalSettingStation::GetInstance().GetResourceDir() /
      "css" / "default.qss";
  QFile file(css_path.u8string().c_str());
  file.open(QFile::ReadOnly);
  QString styleSheet = QLatin1String(file.readAll());
  qApp->setStyleSheet(styleSheet);
  file.close();
#endif

  // init signal station
  UISignalStation::GetInstance();

  // init common utils
  CommonUtils::GetInstance();

  // application proxy configure
  bool proxy_enable =
      GlobalSettingStation::GetInstance().LookupSettings("proxy.enable", false);

  // if enable proxy for application
  if (proxy_enable) {
    try {
      std::string proxy_type =
          GlobalSettingStation::GetInstance().LookupSettings("proxy.proxy_type",
                                                             std::string{});
      std::string proxy_host =
          GlobalSettingStation::GetInstance().LookupSettings("proxy.proxy_host",
                                                             std::string{});
      int proxy_port =
          GlobalSettingStation::GetInstance().LookupSettings("proxy.port", 0);
      std::string proxy_username =
          GlobalSettingStation::GetInstance().LookupSettings("proxy.username",
                                                             std::string{});
      std::string proxy_password =
          GlobalSettingStation::GetInstance().LookupSettings("proxy.password",
                                                             std::string{});
      SPDLOG_DEBUG("proxy settings: type {}, host {}, port: {}", proxy_type,
                   proxy_host, proxy_port);

      QNetworkProxy::ProxyType proxy_type_qt = QNetworkProxy::NoProxy;
      if (proxy_type == "HTTP") {
        proxy_type_qt = QNetworkProxy::HttpProxy;
      } else if (proxy_type == "Socks5") {
        proxy_type_qt = QNetworkProxy::Socks5Proxy;
      } else {
        proxy_type_qt = QNetworkProxy::DefaultProxy;
      }

      // create proxy object and apply settings
      QNetworkProxy proxy;
      if (proxy_type_qt != QNetworkProxy::DefaultProxy) {
        proxy.setType(proxy_type_qt);
        proxy.setHostName(QString::fromStdString(proxy_host));
        proxy.setPort(proxy_port);
        if (!proxy_username.empty()) {
          proxy.setUser(QString::fromStdString(proxy_username));
        }
        if (!proxy_password.empty()) {
          proxy.setPassword(QString::fromStdString(proxy_password));
        }
      } else {
        proxy.setType(proxy_type_qt);
      }
      QNetworkProxy::setApplicationProxy(proxy);

    } catch (...) {
      SPDLOG_ERROR("setting operation error: proxy setings");
      // no proxy by default
      QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    }
  } else {
    // no proxy by default
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
  }

  if (Module::RetrieveRTValueTypedOrDefault<>("core", "env.state.basic",
                                              std::string{"0"}) == "0") {
    WaitEnvCheckingProcess();
  }
}

auto RunGpgFrontendUI(QApplication* app) -> int {
  // create main window and show it
  auto main_window = std::make_unique<GpgFrontend::UI::MainWindow>();

  // pre-check, if application need to restart
  if (CommonUtils::GetInstance()->isApplicationNeedRestart()) {
    SPDLOG_DEBUG("application need to restart, before mian window init");
    return kDeepRestartCode;
  }

  // init main window
  main_window->Init();

  // show main windows
  SPDLOG_DEBUG("main window is ready to show");
  main_window->show();

  // start the main event loop
  return app->exec();
}

void InitUILoggingSystem(spdlog::level::level_enum level) {
  // get the log directory
  auto logfile_path = (GlobalSettingStation::GetInstance().GetLogDir() / "ui");
  logfile_path.replace_extension(".log");

  // sinks
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(GpgFrontend::SecureCreateSharedObject<
                  spdlog::sinks::stderr_color_sink_mt>());
  sinks.push_back(GpgFrontend::SecureCreateSharedObject<
                  spdlog::sinks::rotating_file_sink_mt>(logfile_path.u8string(),
                                                        1048576 * 32, 32));

  // thread pool
  spdlog::init_thread_pool(1024, 2);

  // logger
  auto ui_logger = GpgFrontend::SecureCreateSharedObject<spdlog::async_logger>(
      "ui", begin(sinks), end(sinks), spdlog::thread_pool());
  ui_logger->set_pattern(
      "[%H:%M:%S.%e] [T:%t] [%=6n] %^[%=8l]%$ [%s:%#] [%!] -> %v (+%ius)");

  // set the level of logger
  ui_logger->set_level(level);

  // flush policy
  ui_logger->flush_on(spdlog::level::err);
  spdlog::flush_every(std::chrono::seconds(5));

  // register it as default logger
  spdlog::set_default_logger(ui_logger);
}

void ShutdownUILoggingSystem() {
#ifdef WINDOWS
  // Under VisualStudio, this must be called before main finishes to workaround
  // a known VS issue
  spdlog::drop_all();
  spdlog::shutdown();
#endif
}

void GPGFRONTEND_UI_EXPORT DestroyGpgFrontendUI() { ShutdownUILoggingSystem(); }

/**
 * @brief setup the locale and load the translations
 *
 */
void InitLocale() {
  // get the instance of the GlobalSettingStation
  auto& settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetMainSettings();

  // create general settings if not exist
  if (!settings.exists("general") ||
      settings.lookup("general").getType() != libconfig::Setting::TypeGroup) {
    settings.add("general", libconfig::Setting::TypeGroup);
  }

  // set system default at first
  auto& general = settings["general"];
  if (!general.exists("lang")) {
    general.add("lang", libconfig::Setting::TypeString) = "";
  }

  // sync the settings to the file
  GpgFrontend::GlobalSettingStation::GetInstance().SyncSettings();

  SPDLOG_DEBUG("current system locale: {}", setlocale(LC_ALL, nullptr));

  // read from settings file
  std::string lang;
  if (!general.lookupValue("lang", lang)) {
    SPDLOG_ERROR(_("could not read properly from configure file"));
  };

  SPDLOG_DEBUG("lang from settings: {}", lang);
  SPDLOG_DEBUG("project name: {}", PROJECT_NAME);
  SPDLOG_DEBUG("locales path: {}",
               GpgFrontend::GlobalSettingStation::GetInstance()
                   .GetLocaleDir()
                   .u8string());

#ifndef WINDOWS
  if (!lang.empty()) {
    std::string lc = lang + ".UTF-8";

    // set LC_ALL
    auto* locale_name = setlocale(LC_ALL, lc.c_str());
    if (locale_name == nullptr) SPDLOG_WARN("set LC_ALL failed, lc: {}", lc);
    auto* language = getenv("LANGUAGE");
    // set LANGUAGE
    std::string language_env = language == nullptr ? "en" : language;
    language_env.insert(0, lang + ":");
    SPDLOG_DEBUG("language env: {}", language_env);
    if (setenv("LANGUAGE", language_env.c_str(), 1) != 0) {
      SPDLOG_WARN("set LANGUAGE {} failed", language_env);
    };
  }
#else
  if (!lang.empty()) {
    std::string lc = lang;

    // set LC_ALL
    auto* locale_name = setlocale(LC_ALL, lc.c_str());
    if (locale_name == nullptr) SPDLOG_WARN("set LC_ALL failed, lc: {}", lc);

    auto language = getenv("LANGUAGE");
    // set LANGUAGE
    std::string language_env = language == nullptr ? "en" : language;
    language_env.insert(0, lang + ":");
    language_env.insert(0, "LANGUAGE=");
    SPDLOG_DEBUG("language env: {}", language_env);
    if (putenv(language_env.c_str())) {
      SPDLOG_WARN("set LANGUAGE {} failed", language_env);
    };
  }
#endif

  bindtextdomain(PROJECT_NAME, GpgFrontend::GlobalSettingStation::GetInstance()
                                   .GetLocaleDir()
                                   .u8string()
                                   .c_str());
  bind_textdomain_codeset(PROJECT_NAME, "utf-8");
  textdomain(PROJECT_NAME);
}

}  // namespace GpgFrontend::UI
