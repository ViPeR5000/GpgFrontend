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

#include "ui/widgets/HelpPage.h"

#include "core/function/GlobalSettingStation.h"

namespace GpgFrontend::UI {

HelpPage::HelpPage(const QString& path, QWidget* parent) : QWidget(parent) {
  browser_ = new QTextBrowser();
  auto* main_layout = new QVBoxLayout();
  main_layout->setSpacing(0);
  main_layout->addWidget(browser_);
  main_layout->setContentsMargins(0, 0, 0, 0);
  setLayout(main_layout);

  connect(browser_, &QTextBrowser::anchorClicked, this,
          &HelpPage::slot_open_url);
  browser_->setOpenLinks(false);
  browser_->setSource(localized_help(QUrl(path)));
  browser_->setFocus();
}

void HelpPage::slot_open_url(const QUrl& url) {
  browser_->setSource(localized_help(url));
};

/**
 * @brief HelpPage::localized_help
 * check if the requested file is also available with the locale,
 * e.g. return index.de.html if index.html was requested but the
 * locale is de and index.de.html is available
 * @param url
 * @return
 */
auto HelpPage::localized_help(const QUrl& url) -> QUrl {
  QString path = url.toLocalFile();
  QString filename = path.mid(path.lastIndexOf("/") + 1);
  QString filepath = path.left(path.lastIndexOf("/") + 1);
  QStringList fileparts = filename.split(".");

  // QSettings settings;
  QString lang = GetSettings().value("int/lang", QLocale().name()).toString();
  if (lang.isEmpty()) {
    lang = QLocale().name();
  }

  fileparts.insert(1, lang);
  QString langfile = filepath + fileparts.join(".");

  if (QFile(QUrl(langfile).toLocalFile()).exists()) {
    return langfile;
  }
  return path;
}

QTextBrowser* HelpPage::GetBrowser() { return browser_; }

}  // namespace GpgFrontend::UI
