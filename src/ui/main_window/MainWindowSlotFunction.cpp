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

#include "MainWindow.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/module/ModuleManager.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/help/AboutDialog.h"
#include "ui/dialog/import_export/KeyUploadDialog.h"
#include "ui/dialog/keypair_details/KeyDetailsDialog.h"
#include "ui/function/SetOwnerTrustLevel.h"
#include "ui/widgets/FindWidget.h"
#include "ui/widgets/KeyList.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::UI {

void MainWindow::slot_find() {
  if (edit_->TabCount() == 0 || edit_->CurTextPage() == nullptr) {
    return;
  }

  // At first close verifynotification, if existing
  edit_->SlotCurPageTextEdit()->CloseNoteByClass("FindWidget");

  auto* fw = new FindWidget(this, edit_->CurTextPage());
  edit_->SlotCurPageTextEdit()->ShowNotificationWidget(fw, "FindWidget");
}

/*
 * Append the selected (not checked!) Key(s) To Textedit
 */
void MainWindow::slot_append_selected_keys() {
  auto key_ids = m_key_list_->GetSelected();

  if (key_ids->empty()) {
    FLOG_W("no key is selected to export");
    return;
  }

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (!key.IsGood()) {
    LOG_W() << "selected key for exporting is invalid, key id: "
            << key_ids->front();
    return;
  }

  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance(
                              m_key_list_->GetCurrentGpgContextChannel())
                              .ExportKey(key, false, true, false);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    CommonUtils::RaiseMessageBox(this, err);
    return;
  }

  edit_->SlotAppendText2CurTextPage(gf_buffer.ConvertToQByteArray());
}

void MainWindow::slot_append_keys_create_datetime() {
  auto key_ids = m_key_list_->GetSelected();

  if (key_ids->empty()) {
    FLOG_W("no key is selected");
    return;
  }

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }

  auto create_datetime_format_str_local =
      QLocale().toString(key.GetCreateTime()) + " (" + tr("Localize") + ") " +
      "\n";
  auto create_datetime_format_str =
      QLocale().toString(key.GetCreateTime().toUTC()) + " (" + tr("UTC") +
      ") " + "\n ";
  edit_->SlotAppendText2CurTextPage(create_datetime_format_str_local +
                                    create_datetime_format_str);
}

void MainWindow::slot_append_keys_expire_datetime() {
  auto key_ids = m_key_list_->GetSelected();

  if (key_ids->empty()) {
    FLOG_W("no key is selected");
    return;
  }

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }

  auto expire_datetime_format_str_local =
      QLocale().toString(key.GetExpireTime()) + " (" + tr("Local Time") + ") " +
      "\n";
  auto expire_datetime_format_str =
      QLocale().toString(key.GetExpireTime().toUTC()) + " (UTC) " + "\n";

  edit_->SlotAppendText2CurTextPage(expire_datetime_format_str_local +
                                    expire_datetime_format_str);
}

void MainWindow::slot_append_keys_fingerprint() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }

  auto fingerprint_format_str =
      BeautifyFingerprint(key.GetFingerprint()) + "\n";

  edit_->SlotAppendText2CurTextPage(fingerprint_format_str);
}

void MainWindow::slot_copy_mail_address_to_clipboard() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }
  QClipboard* cb = QApplication::clipboard();
  cb->setText(key.GetEmail());
}

void MainWindow::slot_copy_default_uid_to_clipboard() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }
  QClipboard* cb = QApplication::clipboard();
  cb->setText(key.GetUIDs()->front().GetUID());
}

void MainWindow::slot_copy_key_id_to_clipboard() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (!key.IsGood()) {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
    return;
  }
  QClipboard* cb = QApplication::clipboard();
  cb->setText(key.GetId());
}

void MainWindow::slot_show_key_details() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (key.IsGood()) {
    new KeyDetailsDialog(m_key_list_->GetCurrentGpgContextChannel(), key, this);
  } else {
    QMessageBox::critical(this, tr("Error"), tr("Key Not Found."));
  }
}

void MainWindow::slot_add_key_2_favorite() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  if (!key.IsGood()) return;

  auto key_db_name =
      GetGpgKeyDatabaseName(m_key_list_->GetCurrentGpgContextChannel());

  LOG_D() << "add key" << key.GetId() << "to favorite at key db" << key_db_name;

  CommonUtils::GetInstance()->AddKey2Favorite(key_db_name, key);
  emit SignalUIRefresh();
}

void MainWindow::slot_remove_key_from_favorite() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto key =
      GpgKeyGetter::GetInstance(m_key_list_->GetCurrentGpgContextChannel())
          .GetKey(key_ids->front());
  assert(key.IsGood());

  auto key_db_name =
      GetGpgKeyDatabaseName(m_key_list_->GetCurrentGpgContextChannel());

  CommonUtils::GetInstance()->RemoveKeyFromFavorite(key_db_name, key);

  emit SignalUIRefresh();
}

void MainWindow::refresh_keys_from_key_server() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;
  CommonUtils::GetInstance()->ImportKeyFromKeyServer(
      m_key_list_->GetCurrentGpgContextChannel(), *key_ids);
}

void MainWindow::slot_set_owner_trust_level_of_key() {
  auto key_ids = m_key_list_->GetSelected();
  if (key_ids->empty()) return;

  auto* function = new SetOwnerTrustLevel(this);
  function->Exec(m_key_list_->GetCurrentGpgContextChannel(), key_ids->front());
  function->deleteLater();
}

void MainWindow::upload_key_to_server() {
  auto key_ids = m_key_list_->GetSelected();
  auto* dialog = new KeyUploadDialog(m_key_list_->GetCurrentGpgContextChannel(),
                                     key_ids, this);
  dialog->show();
  dialog->SlotUpload();
}

void MainWindow::SlotOpenFile(const QString& path) {
  edit_->SlotOpenFile(path);
}

void MainWindow::slot_version_upgrade_notify() {
  FLOG_D(
      "slot version upgrade notify called, checking version info from rt...");

  auto is_loading_done = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.loading_done", false);
  if (!is_loading_done) {
    FLOG_W("invalid version info from rt, loading hasn't done yet");
    return;
  }

  auto is_need_upgrade = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.need_upgrade", false);

  auto is_current_a_withdrawn_version = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.current_a_withdrawn_version", false);

  auto is_current_version_released = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.current_version_released", false);

  auto latest_version = Module::RetrieveRTValueTypedOrDefault<>(
      kVersionCheckingModuleID, "version.latest_version", QString{});

  if (is_need_upgrade) {
    statusBar()->showMessage(
        tr("GpgFrontend Upgradeable (New Version: %1).").arg(latest_version),
        30000);
    auto* update_button = new QPushButton("Update GpgFrontend", this);
    connect(update_button, &QPushButton::clicked, [=]() {
      auto* about_dialog = new AboutDialog(tr("Update"), this);
      about_dialog->show();
    });
    statusBar()->addPermanentWidget(update_button, 0);
  } else if (is_current_a_withdrawn_version) {
    QMessageBox::warning(
        this, tr("Withdrawn Version"),

        tr("This version(%1) may have been withdrawn by the developer due "
           "to serious problems. Please stop using this version "
           "immediately and use the latest stable version.")
                .arg(latest_version) +
            "<br/>" +
            tr("You can download the latest stable version(%1) on "
               "Github Releases Page.<br/>")
                .arg(latest_version));
  } else if (!is_current_version_released) {
    statusBar()->showMessage(
        tr("This maybe a BETA Version (Latest Stable Version: %1).")
            .arg(latest_version),
        30000);
  }
}

void MainWindow::slot_refresh_current_file_view() {
  if (edit_->CurFilePage() != nullptr) {
    edit_->CurFilePage()->update();
  }
}

void MainWindow::slot_import_key_from_edit() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) return;

  CommonUtils::GetInstance()->SlotImportKeys(
      this, m_key_list_->GetCurrentGpgContextChannel(),
      edit_->CurTextPage()->GetTextPage()->toPlainText().toLatin1());
}

void MainWindow::slot_verify_email_by_eml_data(const QByteArray& buffer) {
  Module::TriggerEvent(
      "EMAIL_VERIFY_EML_DATA",
      {
          {"eml_data", QString::fromLatin1(buffer.toBase64())},
      },
      [=](Module::EventIdentifier i, Module::Event::ListenerIdentifier ei,
          Module::Event::Params p) {
        LOG_D() << "EMAIL_VERIFY_EML_DATA callback: " << i << ei;
        if (p["ret"] != "0" || !p["error_msg"].isEmpty()) {
          LOG_E() << "An error occurred trying to verify email, "
                  << "error message: " << p["error_msg"]
                  << "reply data: " << p["reply_data"];
        } else if (p.contains("signature") && p.contains("mime")) {
          const auto mime = QByteArray::fromBase64(p["mime"].toLatin1());
          const auto signature =
              QByteArray::fromBase64(p["signature"].toLatin1());

          auto part_mime_content_hash =
              QCryptographicHash::hash(mime, QCryptographicHash::Sha1);
          LOG_D() << "get raw data of mime part, size:" << mime.size()
                  << "sha1 hash:" << part_mime_content_hash.toHex();

          SlotVerify(mime, signature);
        } else {
          LOG_E() << "mime or signature data is missing";
        }
      });
}

void MainWindow::SlotVerifyEML() {
  if (edit_->TabCount() == 0 || edit_->SlotCurPageTextEdit() == nullptr) return;

  auto buffer = edit_->CurTextPage()->GetTextPage()->toPlainText().toLatin1();
  buffer = buffer.replace("\n", "\r\n");

  // LOG_D() << "EML BUFFER: " << buffer;

  slot_verify_email_by_eml_data(buffer);
}

}  // namespace GpgFrontend::UI
