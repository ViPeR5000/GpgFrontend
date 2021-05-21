/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/KeyDetailsDialog.h"

KeyDetailsDialog::KeyDetailsDialog(GpgME::GpgContext *ctx, const GpgKey& key, QWidget *parent)
        : QDialog(parent) {


    tabWidget = new QTabWidget(this);
    tabWidget->addTab(new KeyPairDetailTab(ctx, key, this), tr("KeyPair Details"));

    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tabWidget);

    this->setLayout(mainLayout);
    this->setWindowTitle(tr("Keydetails"));
    this->setModal(true);
    this->show();

    exec();
}