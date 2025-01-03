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

#include "SubkeyGenerateDialog.h"

#include <cassert>
#include <cstddef>

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/utils/CacheUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"

namespace GpgFrontend::UI {

SubkeyGenerateDialog::SubkeyGenerateDialog(int channel, const KeyId& key_id,
                                           QWidget* parent)
    : GeneralDialog(typeid(SubkeyGenerateDialog).name(), parent),
      current_gpg_context_channel_(channel),
      key_(GpgKeyGetter::GetInstance(current_gpg_context_channel_)
               .GetKey(key_id)) {
  assert(key_.IsGood());

  bool longer_expiration_date =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("basic/longer_expiration_date", false)
          .toBool();

  max_date_time_ = longer_expiration_date
                       ? QDateTime::currentDateTime().toLocalTime().addYears(30)
                       : QDateTime::currentDateTime().toLocalTime().addYears(2);

  button_box_ =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  key_usage_group_box_ = create_key_usage_group_box();

  auto* group_grid = new QGridLayout(this);
  group_grid->addWidget(create_basic_info_group_box(), 0, 0);
  group_grid->addWidget(key_usage_group_box_, 1, 0);

  auto* tipps_label =
      new QLabel(tr("Tipps: if the key pair has a passphrase, the subkey's "
                    "passphrase must be equal to it."));
  tipps_label->setWordWrap(true);
  group_grid->addWidget(tipps_label);

  auto* name_list = new QWidget(this);
  name_list->setLayout(group_grid);

  auto* vbox2 = new QVBoxLayout();
  vbox2->addWidget(name_list);
  vbox2->addWidget(error_label_);
  vbox2->addWidget(button_box_);

  this->setWindowTitle(tr("Generate New Subkey"));
  this->setLayout(vbox2);
  this->setAttribute(Qt::WA_DeleteOnClose);
  this->setModal(true);

  set_signal_slot();
  refresh_widgets_state();
}

QGroupBox* SubkeyGenerateDialog::create_key_usage_group_box() {
  auto* group_box = new QGroupBox(this);
  auto* grid = new QGridLayout(this);

  group_box->setTitle(tr("Key Usage"));

  auto* encrypt = new QCheckBox(tr("Encryption"), group_box);
  encrypt->setTristate(false);

  auto* sign = new QCheckBox(tr("Signing"), group_box);
  sign->setTristate(false);

  auto* cert = new QCheckBox(tr("Certification"), group_box);
  cert->setTristate(false);

  auto* auth = new QCheckBox(tr("Authentication"), group_box);
  auth->setTristate(false);

  key_usage_check_boxes_.push_back(encrypt);
  key_usage_check_boxes_.push_back(sign);
  key_usage_check_boxes_.push_back(cert);
  key_usage_check_boxes_.push_back(auth);

  grid->addWidget(encrypt, 0, 0);
  grid->addWidget(sign, 0, 1);
  grid->addWidget(cert, 1, 0);
  grid->addWidget(auth, 1, 1);

  group_box->setLayout(grid);

  return group_box;
}

QGroupBox* SubkeyGenerateDialog::create_basic_info_group_box() {
  error_label_ = new QLabel();
  key_size_spin_box_ = new QSpinBox(this);
  key_type_combo_box_ = new QComboBox(this);
  no_pass_phrase_check_box_ = new QCheckBox(this);

  for (const auto& algo : GenKeyInfo::GetSupportedSubkeyAlgo()) {
    key_type_combo_box_->addItem(std::get<0>(algo));
  }
  if (!GenKeyInfo::GetSupportedSubkeyAlgo().empty()) {
    key_type_combo_box_->setCurrentIndex(0);
  }

  date_edit_ =
      new QDateTimeEdit(QDateTime::currentDateTime().addYears(2), this);
  date_edit_->setMinimumDateTime(QDateTime::currentDateTime());
  date_edit_->setMaximumDateTime(max_date_time_);
  date_edit_->setDisplayFormat("dd/MM/yyyy hh:mm:ss");
  date_edit_->setCalendarPopup(true);
  date_edit_->setEnabled(true);

  expire_check_box_ = new QCheckBox(this);
  expire_check_box_->setCheckState(Qt::Unchecked);

  auto* vbox1 = new QGridLayout;

  vbox1->addWidget(new QLabel(tr("Key Type") + ": "), 0, 0);
  vbox1->addWidget(new QLabel(tr("KeySize (in Bit)") + ": "), 1, 0);
  vbox1->addWidget(new QLabel(tr("Expiration Date") + ": "), 2, 0);
  vbox1->addWidget(new QLabel(tr("Never Expire")), 2, 3);
  vbox1->addWidget(new QLabel(tr("Non Pass Phrase")), 3, 0);

  vbox1->addWidget(key_type_combo_box_, 0, 1);
  vbox1->addWidget(key_size_spin_box_, 1, 1);
  vbox1->addWidget(date_edit_, 2, 1);
  vbox1->addWidget(expire_check_box_, 2, 2);
  vbox1->addWidget(no_pass_phrase_check_box_, 3, 1);

  auto* basic_info_group_box = new QGroupBox();
  basic_info_group_box->setLayout(vbox1);
  basic_info_group_box->setTitle(tr("Basic Information"));

  return basic_info_group_box;
}

void SubkeyGenerateDialog::set_signal_slot() {
  connect(button_box_, &QDialogButtonBox::accepted, this,
          &SubkeyGenerateDialog::slot_key_gen_accept);
  connect(button_box_, &QDialogButtonBox::rejected, this,
          &SubkeyGenerateDialog::reject);

  connect(expire_check_box_, &QCheckBox::stateChanged, this,
          &SubkeyGenerateDialog::slot_expire_box_changed);

  connect(key_usage_check_boxes_[0], &QCheckBox::stateChanged, this,
          &SubkeyGenerateDialog::slot_encryption_box_changed);
  connect(key_usage_check_boxes_[1], &QCheckBox::stateChanged, this,
          &SubkeyGenerateDialog::slot_signing_box_changed);
  connect(key_usage_check_boxes_[2], &QCheckBox::stateChanged, this,
          &SubkeyGenerateDialog::slot_certification_box_changed);
  connect(key_usage_check_boxes_[3], &QCheckBox::stateChanged, this,
          &SubkeyGenerateDialog::slot_authentication_box_changed);

  connect(key_type_combo_box_, qOverload<int>(&QComboBox::currentIndexChanged),
          this, &SubkeyGenerateDialog::slot_activated_key_type);

  connect(no_pass_phrase_check_box_, &QCheckBox::stateChanged, this,
          [this](int state) -> void {
            gen_key_info_->SetNonPassPhrase(state != 0);
          });
}

void SubkeyGenerateDialog::slot_expire_box_changed() {
  if (expire_check_box_->checkState() != 0U) {
    date_edit_->setEnabled(false);
  } else {
    date_edit_->setEnabled(true);
  }
}

void SubkeyGenerateDialog::refresh_widgets_state() {
  if (gen_key_info_->IsAllowEncryption()) {
    key_usage_check_boxes_[0]->setCheckState(Qt::CheckState::Checked);
  } else {
    key_usage_check_boxes_[0]->setCheckState(Qt::CheckState::Unchecked);
  }

  if (gen_key_info_->IsAllowChangeEncryption()) {
    key_usage_check_boxes_[0]->setDisabled(false);
  } else {
    key_usage_check_boxes_[0]->setDisabled(true);
  }

  if (gen_key_info_->IsAllowSigning()) {
    key_usage_check_boxes_[1]->setCheckState(Qt::CheckState::Checked);
  } else {
    key_usage_check_boxes_[1]->setCheckState(Qt::CheckState::Unchecked);
  }

  if (gen_key_info_->IsAllowChangeSigning()) {
    key_usage_check_boxes_[1]->setDisabled(false);
  } else {
    key_usage_check_boxes_[1]->setDisabled(true);
  }

  if (gen_key_info_->IsAllowCertification()) {
    key_usage_check_boxes_[2]->setCheckState(Qt::CheckState::Checked);
  } else {
    key_usage_check_boxes_[2]->setCheckState(Qt::CheckState::Unchecked);
  }

  if (gen_key_info_->IsAllowChangeCertification()) {
    key_usage_check_boxes_[2]->setDisabled(false);
  } else {
    key_usage_check_boxes_[2]->setDisabled(true);
  }

  if (gen_key_info_->IsAllowAuthentication()) {
    key_usage_check_boxes_[3]->setCheckState(Qt::CheckState::Checked);
  } else {
    key_usage_check_boxes_[3]->setCheckState(Qt::CheckState::Unchecked);
  }

  if (gen_key_info_->IsAllowChangeAuthentication()) {
    key_usage_check_boxes_[3]->setDisabled(false);
  } else {
    key_usage_check_boxes_[3]->setDisabled(true);
  }

  if (gen_key_info_->GetSuggestMinKeySize() == -1 ||
      gen_key_info_->GetSuggestMaxKeySize() == -1) {
    key_size_spin_box_->setDisabled(true);
    key_size_spin_box_->setRange(0, 0);
    key_size_spin_box_->setValue(0);
    key_size_spin_box_->setSingleStep(0);
  } else {
    key_size_spin_box_->setDisabled(false);
    key_size_spin_box_->setRange(gen_key_info_->GetSuggestMinKeySize(),
                                 gen_key_info_->GetSuggestMaxKeySize());
    key_size_spin_box_->setValue(gen_key_info_->GetKeyLength());
    key_size_spin_box_->setSingleStep(gen_key_info_->GetSizeChangeStep());
  }
}

void SubkeyGenerateDialog::slot_key_gen_accept() {
  QString buffer;
  QTextStream err_stream(&buffer);

  /**
   * primary keys should have a reasonable expiration date (no more than 2 years
   * in the future)
   */
  if (date_edit_->dateTime() > QDateTime::currentDateTime().addYears(2)) {
    err_stream << "  " << tr("Expiration time no more than 2 years.") << "  ";
  }

  auto err_string = err_stream.readAll();

  if (err_string.isEmpty()) {
    gen_key_info_->SetKeyLength(key_size_spin_box_->value());

    if (expire_check_box_->checkState() != 0U) {
      gen_key_info_->SetNonExpired(true);
    } else {
      gen_key_info_->SetExpireTime(date_edit_->dateTime());
    }

    CommonUtils::WaitForOpera(
        this, tr("Generating"),
        [this, key = this->key_,
         gen_key_info = this->gen_key_info_](const OperaWaitingHd& hd) {
          GpgKeyOpera::GetInstance(current_gpg_context_channel_)
              .GenerateSubkey(key, gen_key_info,
                              [this, hd](GpgError err, const DataObjectPtr&) {
                                // stop showing waiting dialog
                                hd();

                                if (CheckGpgError(err) == GPG_ERR_USER_1) {
                                  QMessageBox::critical(
                                      this, tr("Error"),
                                      tr("Unknown error occurred"));
                                  return;
                                }

                                CommonUtils::RaiseMessageBox(this, err);
                                if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
                                  emit UISignalStation::GetInstance()
                                      -> SignalKeyDatabaseRefresh();
                                }
                              });
        });
    this->done(0);

  } else {
    /**
     * create error message
     */
    error_label_->setAutoFillBackground(true);
    QPalette error = error_label_->palette();
    error.setColor(QPalette::Window, "#ff8080");
    error_label_->setPalette(error);
    error_label_->setText(err_string);

    this->show();
  }
}

void SubkeyGenerateDialog::slot_encryption_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowEncryption(false);
  } else {
    gen_key_info_->SetAllowEncryption(true);
  }
}

void SubkeyGenerateDialog::slot_signing_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowSigning(false);
  } else {
    gen_key_info_->SetAllowSigning(true);
  }
}

void SubkeyGenerateDialog::slot_certification_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowCertification(false);
  } else {
    gen_key_info_->SetAllowCertification(true);
  }
}

void SubkeyGenerateDialog::slot_authentication_box_changed(int state) {
  if (state == 0) {
    gen_key_info_->SetAllowAuthentication(false);
  } else {
    gen_key_info_->SetAllowAuthentication(true);
  }
}

void SubkeyGenerateDialog::slot_activated_key_type(int index) {
  // check
  assert(gen_key_info_->GetSupportedSubkeyAlgo().size() >
         static_cast<size_t>(index));
  gen_key_info_->SetAlgo(
      std::get<2>(gen_key_info_->GetSupportedSubkeyAlgo()[index]));
  refresh_widgets_state();
}

}  // namespace GpgFrontend::UI
