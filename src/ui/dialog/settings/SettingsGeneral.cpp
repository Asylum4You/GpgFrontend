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

#include "SettingsGeneral.h"

#include "SettingsDialog.h"
#include "core/GpgModel.h"
#include "core/function/GlobalSettingStation.h"
#include "ui_GeneralSettings.h"

namespace GpgFrontend::UI {

GeneralTab::GeneralTab(QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_GeneralSettings>()) {
  ui_->setupUi(this);

  ui_->cacheBox->setTitle(_("Cache"));
  ui_->clearGpgPasswordCacheCheckBox->setText(
      _("Clear gpg password cache when closing GpgFrontend."));
  ui_->restoreTextEditorPageCheckBox->setText(
      _("Automatically restore unsaved Text Editor pages after an application "
        "crash."));

  ui_->importConfirmationBox->setTitle(_("Operation"));
  ui_->longerKeyExpirationDateCheckBox->setText(
      _("Enable to use longer key expiration date."));
  ui_->importConfirmationCheckBox->setText(
      _("Import files dropped on the Key List without confirmation."));

  ui_->langBox->setTitle(_("Language"));
  ui_->langNoteLabel->setText(
      "<b>" + QString(_("NOTE")) + _(": ") + "</b>" +
      _("GpgFrontend will restart automatically if you change the language!"));

  ui_->dataBox->setTitle(_("Data"));
  ui_->clearAllLogFilesButton->setText(
      QString(_("Clear All Log (Total Size: %1)")) %
      GlobalSettingStation::GetInstance().GetLogFilesSize());
  ui_->clearAllDataObjectsButton->setText(
      QString(_("Clear All Data Objects (Total Size: %1)"))
          .arg(GlobalSettingStation::GetInstance().GetDataObjectsFilesSize()));

  lang_ = SettingsDialog::ListLanguages();
  for (const auto& l : lang_) {
    ui_->langSelectBox->addItem(l);
  }
  connect(ui_->langSelectBox, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &GeneralTab::slot_language_changed);

  connect(ui_->clearAllLogFilesButton, &QPushButton::clicked, this, [=]() {
    GlobalSettingStation::GetInstance().ClearAllLogFiles();
    ui_->clearAllLogFilesButton->setText(
        QString(_("Clear All Log (Total Size: %1)"))
            .arg(GlobalSettingStation::GetInstance().GetLogFilesSize()));
  });

  connect(ui_->clearAllDataObjectsButton, &QPushButton::clicked, this, [=]() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this, _("Confirm"),
        _("Are you sure you want to clear all data objects?\nThis will result "
          "in "
          "loss of all cached form positions, statuses, key servers, etc."),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
      GlobalSettingStation::GetInstance().ClearAllDataObjects();
      ui_->clearAllDataObjectsButton->setText(
          QString(_("Clear All Data Objects (Total Size: %1)"))
              .arg(GlobalSettingStation::GetInstance()
                       .GetDataObjectsFilesSize()));
    }
  });

  SetSettings();
}

void GeneralTab::SetSettings() {
  auto settings = GlobalSettingStation::GetInstance().GetSettings();

  bool clear_gpg_password_cache =
      settings.value("basic/clear_gpg_password_cache", true).toBool();
  ui_->clearGpgPasswordCacheCheckBox->setCheckState(
      clear_gpg_password_cache ? Qt::Checked : Qt::Unchecked);

  bool restore_text_editor_page =
      settings.value("basic/restore_text_editor_page", true).toBool();
  ui_->restoreTextEditorPageCheckBox->setCheckState(
      restore_text_editor_page ? Qt::Checked : Qt::Unchecked);

  bool longer_expiration_date =
      settings.value("basic/longer_expiration_date", false).toBool();
  ui_->longerKeyExpirationDateCheckBox->setCheckState(
      longer_expiration_date ? Qt::Checked : Qt::Unchecked);

  bool confirm_import_keys =
      settings.value("basic/confirm_import_keys", false).toBool();
  ui_->importConfirmationCheckBox->setCheckState(
      confirm_import_keys ? Qt::Checked : Qt::Unchecked);

  QString lang_key = settings.value("basic/lang").toString();
  QString lang_value = lang_.value(lang_key);
  GF_UI_LOG_DEBUG("lang settings current: {}", lang_value.toStdString());
  if (!lang_.empty()) {
    ui_->langSelectBox->setCurrentIndex(
        ui_->langSelectBox->findText(lang_value));
  } else {
    ui_->langSelectBox->setCurrentIndex(0);
  }
}

void GeneralTab::ApplySettings() {
  auto settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetSettings();

  settings.setValue("basic/longer_expiration_date",
                    ui_->longerKeyExpirationDateCheckBox->isChecked());
  settings.setValue("basic/clear_gpg_password_cache",
                    ui_->clearGpgPasswordCacheCheckBox->isChecked());
  settings.setValue("basic/restore_text_editor_page",
                    ui_->restoreTextEditorPageCheckBox->isChecked());
  settings.setValue("basic/confirm_import_keys",
                    ui_->importConfirmationCheckBox->isChecked());
  settings.setValue("basic/lang", lang_.key(ui_->langSelectBox->currentText()));
}

void GeneralTab::slot_language_changed() { emit SignalRestartNeeded(true); }

}  // namespace GpgFrontend::UI
