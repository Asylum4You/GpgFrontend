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

#include "ui/widgets/KeyList.h"

#include <cstddef>
#include <mutex>

#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "ui/UISignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/dialog/import_export/KeyImportDetailDialog.h"
#include "ui_KeyList.h"

namespace GpgFrontend::UI {

KeyList::KeyList(KeyMenuAbility::AbilityType menu_ability, QWidget* parent)
    : QWidget(parent),
      ui_(GpgFrontend::SecureCreateSharedObject<Ui_KeyList>()),
      menu_ability_(menu_ability) {
  init();
}

void KeyList::init() {
  ui_->setupUi(this);

  ui_->menuWidget->setHidden(menu_ability_ == 0U);
  ui_->refreshKeyListButton->setHidden(~menu_ability_ &
                                       KeyMenuAbility::REFRESH);
  ui_->syncButton->setHidden(~menu_ability_ & KeyMenuAbility::SYNC_PUBLIC_KEY);
  ui_->uncheckButton->setHidden(~menu_ability_ & KeyMenuAbility::UNCHECK_ALL);
  ui_->searchBarEdit->setHidden(~menu_ability_ & KeyMenuAbility::SEARCH_BAR);

  ui_->keyGroupTab->clear();
  popup_menu_ = new QMenu(this);

  bool forbid_all_gnupg_connection =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("network/forbid_all_gnupg_connection", false)
          .toBool();

  // forbidden networks connections
  if (forbid_all_gnupg_connection) ui_->syncButton->setDisabled(true);

  // register key database refresh signal
  connect(this, &KeyList::SignalRefreshDatabase, UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefresh);
  connect(UISignalStation::GetInstance(),
          &UISignalStation::SignalKeyDatabaseRefreshDone, this,
          &KeyList::SlotRefresh);
  connect(UISignalStation::GetInstance(), &UISignalStation::SignalUIRefresh,
          this, &KeyList::SlotRefreshUI);

  // register key database sync signal for refresh button
  connect(ui_->refreshKeyListButton, &QPushButton::clicked, this,
          &KeyList::SignalRefreshDatabase);

  connect(ui_->uncheckButton, &QPushButton::clicked, this,
          &KeyList::uncheck_all);
  connect(ui_->checkALLButton, &QPushButton::clicked, this,
          &KeyList::check_all);
  connect(ui_->syncButton, &QPushButton::clicked, this,
          &KeyList::slot_sync_with_key_server);
  connect(ui_->searchBarEdit, &QLineEdit::textChanged, this,
          &KeyList::filter_by_keyword);
  connect(this, &KeyList::SignalRefreshStatusBar,
          UISignalStation::GetInstance(),
          &UISignalStation::SignalRefreshStatusBar);

  setAcceptDrops(true);

  ui_->refreshKeyListButton->setText(tr("Refresh"));
  ui_->refreshKeyListButton->setToolTip(
      tr("Refresh the key list to synchronize changes."));
  ui_->syncButton->setText(tr("Sync Public Key"));
  ui_->syncButton->setToolTip(
      tr("Sync public key with your default keyserver."));
  ui_->uncheckButton->setText(tr("Uncheck ALL"));
  ui_->uncheckButton->setToolTip(
      tr("Cancel all checked items in the current tab at once."));
  ui_->checkALLButton->setText(tr("Check ALL"));
  ui_->checkALLButton->setToolTip(
      tr("Check all items in the current tab at once"));
  ui_->searchBarEdit->setPlaceholderText(tr("Search for keys..."));
}

void KeyList::AddListGroupTab(const QString& name, const QString& id,
                              KeyListRow::KeyType selectType,
                              KeyListColumn::InfoType infoType,
                              const KeyTable::KeyTableFilter filter) {
  GF_UI_LOG_DEBUG("add list group tab: {}", name);

  auto* key_list = new QTableWidget(this);
  if (m_key_list_ == nullptr) {
    m_key_list_ = key_list;
  }
  key_list->setObjectName(id);
  ui_->keyGroupTab->addTab(key_list, name);
  m_key_tables_.emplace_back(key_list, selectType, infoType, filter);
  m_key_tables_.back().SetMenuAbility(menu_ability_);

  key_list->setColumnCount(8);
  key_list->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  key_list->verticalHeader()->hide();
  key_list->setShowGrid(false);
  key_list->sortByColumn(2, Qt::AscendingOrder);
  key_list->setSelectionBehavior(QAbstractItemView::SelectRows);
  key_list->setSelectionMode(QAbstractItemView::SingleSelection);

  // table items not editable
  key_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
  // no focus (rectangle around table items)
  // maybe it should focus on whole row
  key_list->setFocusPolicy(Qt::NoFocus);

  key_list->setAlternatingRowColors(true);

  // Hidden Column For Purpose
  if ((infoType & KeyListColumn::TYPE) == 0U) {
    key_list->setColumnHidden(1, true);
  }
  if ((infoType & KeyListColumn::NAME) == 0U) {
    key_list->setColumnHidden(2, true);
  }
  if ((infoType & KeyListColumn::EmailAddress) == 0U) {
    key_list->setColumnHidden(3, true);
  }
  if ((infoType & KeyListColumn::Usage) == 0U) {
    key_list->setColumnHidden(4, true);
  }
  if ((infoType & KeyListColumn::Validity) == 0U) {
    key_list->setColumnHidden(5, true);
  }
  if ((infoType & KeyListColumn::KeyID) == 0U) {
    key_list->setColumnHidden(6, true);
  }
  if ((infoType & KeyListColumn::FingerPrint) == 0U) {
    key_list->setColumnHidden(7, true);
  }

  QStringList labels;
  labels << tr("Select") << tr("Type") << tr("Name") << tr("Email Address")
         << tr("Usage") << tr("Trust") << tr("Key ID") << tr("Finger Print");

  key_list->setHorizontalHeaderLabels(labels);
  key_list->horizontalHeader()->setStretchLastSection(false);

  connect(key_list, &QTableWidget::doubleClicked, this,
          &KeyList::slot_double_clicked);
}

void KeyList::SlotRefresh() {
  GF_UI_LOG_DEBUG("refresh, address: {}", static_cast<void*>(this));

  ui_->refreshKeyListButton->setDisabled(true);
  ui_->syncButton->setDisabled(true);

  emit SignalRefreshStatusBar(tr("Refreshing Key List..."), 3000);
  this->buffered_keys_list_ = GpgKeyGetter::GetInstance().FetchKey();
  this->slot_refresh_ui();
}

void KeyList::SlotRefreshUI() {
  GF_UI_LOG_DEBUG("refresh, address: {}", static_cast<void*>(this));
  this->slot_refresh_ui();
}

auto KeyList::GetChecked(const KeyTable& key_table) -> KeyIdArgsListPtr {
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_table.key_list_->rowCount(); i++) {
    if (key_table.key_list_->item(i, 0)->checkState() == Qt::Checked) {
      ret->push_back(key_table.buffered_keys_[i].GetId());
    }
  }
  return ret;
}

auto KeyList::GetChecked() -> KeyIdArgsListPtr {
  auto* key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 0)->checkState() == Qt::Checked) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

auto KeyList::GetAllPrivateKeys() -> KeyIdArgsListPtr {
  auto* key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;
  auto ret = std::make_unique<KeyIdArgsList>();
  for (int i = 0; i < key_list->rowCount(); i++) {
    if ((key_list->item(i, 1) != nullptr) && buffered_keys[i].IsPrivateKey()) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

auto KeyList::GetCheckedPrivateKey() -> KeyIdArgsListPtr {
  auto ret = std::make_unique<KeyIdArgsList>();
  if (ui_->keyGroupTab->size().isEmpty()) return ret;

  auto* key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;

  for (int i = 0; i < key_list->rowCount(); i++) {
    if ((key_list->item(i, 0)->checkState() == Qt::Checked) &&
        ((key_list->item(i, 1)) != nullptr) &&
        buffered_keys[i].IsPrivateKey()) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

auto KeyList::GetCheckedPublicKey() -> KeyIdArgsListPtr {
  auto ret = std::make_unique<KeyIdArgsList>();
  if (ui_->keyGroupTab->size().isEmpty()) return ret;

  auto* key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;

  for (int i = 0; i < key_list->rowCount(); i++) {
    if ((key_list->item(i, 0)->checkState() == Qt::Checked) &&
        ((key_list->item(i, 1)) != nullptr) &&
        !buffered_keys[i].IsPrivateKey()) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

void KeyList::SetChecked(const KeyIdArgsListPtr& keyIds,
                         const KeyTable& key_table) {
  if (!keyIds->empty()) {
    for (int i = 0; i < key_table.key_list_->rowCount(); i++) {
      if (std::find(keyIds->begin(), keyIds->end(),
                    key_table.buffered_keys_[i].GetId()) != keyIds->end()) {
        key_table.key_list_->item(i, 0)->setCheckState(Qt::Checked);
      }
    }
  }
}

void KeyList::SetChecked(KeyIdArgsListPtr key_ids) {
  auto* key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  if (key_list == nullptr) return;
  if (!m_key_tables_.empty()) {
    for (auto& key_table : m_key_tables_) {
      if (key_table.key_list_ == key_list) {
        key_table.SetChecked(std::move(key_ids));
        break;
      }
    }
  }
}

KeyIdArgsListPtr KeyList::GetSelected() {
  auto ret = std::make_unique<KeyIdArgsList>();
  if (ui_->keyGroupTab->size().isEmpty()) return ret;

  auto* key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;

  for (int i = 0; i < key_list->rowCount(); i++) {
    if (key_list->item(i, 0)->isSelected()) {
      ret->push_back(buffered_keys[i].GetId());
    }
  }
  return ret;
}

[[maybe_unused]] auto KeyList::ContainsPrivateKeys() -> bool {
  if (ui_->keyGroupTab->size().isEmpty()) return false;
  m_key_list_ = qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());

  for (int i = 0; i < m_key_list_->rowCount(); i++) {
    if (m_key_list_->item(i, 1) != nullptr) {
      return true;
    }
  }
  return false;
}

void KeyList::SetColumnWidth(int row, int size) {
  if (ui_->keyGroupTab->size().isEmpty()) return;
  m_key_list_ = qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());

  m_key_list_->setColumnWidth(row, size);
}

void KeyList::contextMenuEvent(QContextMenuEvent* event) {
  if (ui_->keyGroupTab->size().isEmpty()) return;
  m_key_list_ = qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());

  QString current_tab_widget_obj_name =
      ui_->keyGroupTab->widget(ui_->keyGroupTab->currentIndex())->objectName();
  GF_UI_LOG_DEBUG("current tab widget object name: {}",
                  current_tab_widget_obj_name);
  if (current_tab_widget_obj_name == "favourite") {
    QList<QAction*> actions = popup_menu_->actions();
    for (QAction* action : actions) {
      if (action->data().toString() == "remove_key_from_favourtie_action") {
        action->setVisible(true);
      } else if (action->data().toString() == "add_key_2_favourite_action") {
        action->setVisible(false);
      }
    }
  } else {
    QList<QAction*> actions = popup_menu_->actions();
    for (QAction* action : actions) {
      if (action->data().toString() == "remove_key_from_favourtie_action") {
        action->setVisible(false);
      } else if (action->data().toString() == "add_key_2_favourite_action") {
        action->setVisible(true);
      }
    }
  }

  if (m_key_list_->selectedItems().length() > 0) {
    popup_menu_->exec(event->globalPos());
  }
}

void KeyList::AddSeparator() { popup_menu_->addSeparator(); }

void KeyList::AddMenuAction(QAction* act) { popup_menu_->addAction(act); }

void KeyList::dropEvent(QDropEvent* event) {
  auto* dialog = new QDialog();

  dialog->setWindowTitle(tr("Import Keys"));
  QLabel* label;
  label = new QLabel(tr("You've dropped something on the table.") + "\n " +
                     tr("GpgFrontend will now try to import key(s).") + "\n");

  // "always import keys"-CheckBox
  auto* check_box = new QCheckBox(tr("Always import without bothering."));

  bool confirm_import_keys = GlobalSettingStation::GetInstance()
                                 .GetSettings()
                                 .value("basic/confirm_import_keys", true)
                                 .toBool();
  if (confirm_import_keys) check_box->setCheckState(Qt::Checked);

  // Buttons for ok and cancel
  auto* button_box =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(button_box, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
  connect(button_box, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

  auto* vbox = new QVBoxLayout();
  vbox->addWidget(label);
  vbox->addWidget(check_box);
  vbox->addWidget(button_box);

  dialog->setLayout(vbox);

  if (confirm_import_keys) {
    dialog->exec();
    if (dialog->result() == QDialog::Rejected) return;

    auto settings = GlobalSettingStation::GetInstance().GetSettings();
    settings.setValue("basic/confirm_import_keys", check_box->isChecked());
  }

  if (event->mimeData()->hasUrls()) {
    for (const QUrl& tmp : event->mimeData()->urls()) {
      QFile file;
      file.setFileName(tmp.toLocalFile());
      if (!file.open(QIODevice::ReadOnly)) {
        GF_UI_LOG_ERROR("couldn't open file: {}", tmp.toString());
      }
      QByteArray in_buffer = file.readAll();
      this->import_keys(in_buffer);
      file.close();
    }
  } else {
    QByteArray in_buffer(event->mimeData()->text().toUtf8());
    this->import_keys(in_buffer);
  }
}

void KeyList::dragEnterEvent(QDragEnterEvent* event) {
  event->acceptProposedAction();
}

void KeyList::import_keys(const QByteArray& in_buffer) {
  auto result =
      GpgKeyImportExporter::GetInstance().ImportKey(GFBuffer(in_buffer));
  (new KeyImportDetailDialog(result, this));
}

void KeyList::slot_double_clicked(const QModelIndex& index) {
  if (ui_->keyGroupTab->size().isEmpty()) return;
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;
  if (m_action_ != nullptr) {
    const auto key =
        GpgKeyGetter::GetInstance().GetKey(buffered_keys[index.row()].GetId());
    m_action_(key, this);
  }
}

void KeyList::SetDoubleClickedAction(
    std::function<void(const GpgKey&, QWidget*)> action) {
  this->m_action_ = std::move(action);
}

auto KeyList::GetSelectedKey() -> QString {
  if (ui_->keyGroupTab->size().isEmpty()) return {};
  const auto& buffered_keys =
      m_key_tables_[ui_->keyGroupTab->currentIndex()].buffered_keys_;

  for (int i = 0; i < m_key_list_->rowCount(); i++) {
    if (m_key_list_->item(i, 0)->isSelected()) {
      return buffered_keys[i].GetId();
    }
  }
  return {};
}

void KeyList::slot_refresh_ui() {
  GF_UI_LOG_DEBUG("refresh: {}", static_cast<void*>(buffered_keys_list_.get()));
  if (buffered_keys_list_ != nullptr) {
    std::lock_guard<std::mutex> guard(buffered_key_list_mutex_);

    for (auto& key_table : m_key_tables_) {
      key_table.Refresh(
          GpgKeyGetter::GetInstance().GetKeysCopy(buffered_keys_list_));
    }
  }
  emit SignalRefreshStatusBar(tr("Key List Refreshed."), 1000);
  ui_->refreshKeyListButton->setDisabled(false);
  ui_->syncButton->setDisabled(false);
}

void KeyList::slot_sync_with_key_server() {
  auto checked_public_keys = GetCheckedPublicKey();

  KeyIdArgsList key_ids;
  if (checked_public_keys->empty()) {
    QMessageBox::StandardButton const reply = QMessageBox::question(
        this, QCoreApplication::tr("Sync All Public Key"),
        QCoreApplication::tr("You have not checked any public keys that you "
                             "want to synchronize, do you want to synchronize "
                             "all local public keys from the key server?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) return;
    {
      std::lock_guard<std::mutex> guard(buffered_key_list_mutex_);
      for (const auto& key : *buffered_keys_list_) {
        if (!(key.IsPrivateKey() && key.IsHasMasterKey())) {
          key_ids.push_back(key.GetId());
        }
      }
    }
  } else {
    key_ids = *checked_public_keys;
  }

  if (key_ids.empty()) return;

  ui_->refreshKeyListButton->setDisabled(true);
  ui_->syncButton->setDisabled(true);

  emit SignalRefreshStatusBar(tr("Syncing Key List..."), 3000);
  CommonUtils::SlotImportKeyFromKeyServer(
      key_ids, [=](const QString& key_id, const QString& status,
                   size_t current_index, size_t all_index) {
        GF_UI_LOG_DEBUG("import key: {} {} {} {}", key_id, status,
                        current_index, all_index);
        auto key = GpgKeyGetter::GetInstance().GetKey(key_id);

        auto status_str = tr("Sync [%1/%2] %3 %4")
                              .arg(current_index)
                              .arg(all_index)
                              .arg(key.GetUIDs()->front().GetUID())
                              .arg(status);
        emit SignalRefreshStatusBar(status_str, 1500);

        if (current_index == all_index) {
          ui_->syncButton->setDisabled(false);
          ui_->refreshKeyListButton->setDisabled(false);
          emit SignalRefreshStatusBar(tr("Key List Sync Done."), 3000);
          emit this->SignalRefreshDatabase();
        }
      });
}

void KeyList::filter_by_keyword() {
  auto keyword = ui_->searchBarEdit->text();
  keyword = keyword.trimmed();

  GF_UI_LOG_DEBUG("get new keyword of search bar: {}", keyword);
  for (auto& table : m_key_tables_) {
    // refresh arguments
    table.SetFilterKeyword(keyword.toLower());
    table.SetMenuAbility(menu_ability_);
  }
  // refresh ui
  SlotRefreshUI();
}

void KeyList::uncheck_all() {
  auto* key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  if (key_list == nullptr) return;
  if (!m_key_tables_.empty()) {
    for (auto& key_table : m_key_tables_) {
      if (key_table.key_list_ == key_list) {
        key_table.UncheckALL();
        break;
      }
    }
  }
}

void KeyList::check_all() {
  auto* key_list =
      qobject_cast<QTableWidget*>(ui_->keyGroupTab->currentWidget());
  if (key_list == nullptr) return;
  if (!m_key_tables_.empty()) {
    for (auto& key_table : m_key_tables_) {
      if (key_table.key_list_ == key_list) {
        key_table.CheckALL();
        break;
      }
    }
  }
}

KeyIdArgsListPtr& KeyTable::GetChecked() {
  if (checked_key_ids_ == nullptr) {
    checked_key_ids_ = std::make_unique<KeyIdArgsList>();
  }
  auto& ret = checked_key_ids_;
  for (size_t i = 0; i < buffered_keys_.size(); i++) {
    auto key_id = buffered_keys_[i].GetId();
    if (key_list_->item(i, 0)->checkState() == Qt::Checked &&
        std::find(ret->begin(), ret->end(), key_id) == ret->end()) {
      ret->push_back(key_id);
    }
  }
  return ret;
}

void KeyTable::SetChecked(KeyIdArgsListPtr key_ids) {
  checked_key_ids_ = std::move(key_ids);
}

void KeyTable::Refresh(KeyLinkListPtr m_keys) {
  auto& checked_key_list = GetChecked();
  // while filling the table, sort enabled causes errors

  key_list_->setSortingEnabled(false);
  key_list_->clearContents();

  // Optimization for copy
  KeyLinkListPtr keys = nullptr;
  if (m_keys == nullptr) {
    keys = GpgKeyGetter::GetInstance().FetchKey();
  } else {
    keys = std::move(m_keys);
  }

  auto it = keys->begin();
  int row_count = 0;

  while (it != keys->end()) {
    // filter by search bar's keyword
    if (ability_ & KeyMenuAbility::SEARCH_BAR && !keyword_.isEmpty()) {
      QStringList infos;
      infos << it->GetName().toLower() << it->GetEmail().toLower()
            << it->GetComment().toLower() << it->GetFingerprint().toLower();

      auto subkeys = it->GetSubKeys();
      for (const auto& subkey : *subkeys) {
        infos << subkey.GetFingerprint().toLower() << subkey.GetID().toLower();
      }

      if (infos.filter(keyword_.toLower()).isEmpty()) {
        it = keys->erase(it);
        continue;
      }
    }

    if (filter_ != nullptr) {
      if (!filter_(*it, *this)) {
        it = keys->erase(it);
        continue;
      }
    }

    if (select_type_ == KeyListRow::ONLY_SECRET_KEY && !it->IsPrivateKey()) {
      it = keys->erase(it);
      continue;
    }
    row_count++;
    it++;
  }

  key_list_->setRowCount(row_count);

  int row_index = 0;
  it = keys->begin();

  buffered_keys_.clear();

  while (it != keys->end()) {
    auto* tmp0 = new QTableWidgetItem(QString::number(row_index));
    tmp0->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable);
    tmp0->setTextAlignment(Qt::AlignCenter);
    tmp0->setCheckState(Qt::Unchecked);
    key_list_->setItem(row_index, 0, tmp0);

    QString type_str;
    QTextStream type_steam(&type_str);
    if (it->IsPrivateKey()) {
      type_steam << "pub/sec";
    } else {
      type_steam << "pub";
    }

    if (it->IsPrivateKey() && !it->IsHasMasterKey()) {
      type_steam << "#";
    }

    if (it->IsHasCardKey()) {
      type_steam << "^";
    }

    auto* tmp1 = new QTableWidgetItem(type_str);
    key_list_->setItem(row_index, 1, tmp1);

    auto* tmp2 = new QTableWidgetItem(it->GetName());
    key_list_->setItem(row_index, 2, tmp2);
    auto* tmp3 = new QTableWidgetItem(it->GetEmail());
    key_list_->setItem(row_index, 3, tmp3);

    QString usage;
    QTextStream usage_steam(&usage);

    if (it->IsHasActualCertificationCapability()) usage_steam << "C";
    if (it->IsHasActualEncryptionCapability()) usage_steam << "E";
    if (it->IsHasActualSigningCapability()) usage_steam << "S";
    if (it->IsHasActualAuthenticationCapability()) usage_steam << "A";

    auto* temp_usage = new QTableWidgetItem(usage);
    temp_usage->setTextAlignment(Qt::AlignCenter);
    key_list_->setItem(row_index, 4, temp_usage);

    auto* temp_validity = new QTableWidgetItem(it->GetOwnerTrust());
    temp_validity->setTextAlignment(Qt::AlignCenter);
    key_list_->setItem(row_index, 5, temp_validity);

    auto* temp_id = new QTableWidgetItem(it->GetId());
    temp_id->setTextAlignment(Qt::AlignCenter);
    key_list_->setItem(row_index, 6, temp_id);

    auto* temp_fpr = new QTableWidgetItem(it->GetFingerprint());
    temp_fpr->setTextAlignment(Qt::AlignCenter);
    key_list_->setItem(row_index, 7, temp_fpr);

    QFont font = tmp2->font();

    // strike out expired keys
    if (it->IsExpired() || it->IsRevoked()) font.setStrikeOut(true);
    if (it->IsPrivateKey()) font.setBold(true);

    tmp0->setFont(font);
    temp_usage->setFont(font);
    temp_fpr->setFont(font);
    temp_validity->setFont(font);
    tmp1->setFont(font);
    tmp2->setFont(font);
    tmp3->setFont(font);
    temp_id->setFont(font);

    // move to buffered keys
    buffered_keys_.emplace_back(std::move(*it));

    it++;
    ++row_index;
  }

  if (!checked_key_list->empty()) {
    for (int i = 0; i < key_list_->rowCount(); i++) {
      if (std::find(checked_key_list->begin(), checked_key_list->end(),
                    buffered_keys_[i].GetId()) != checked_key_list->end()) {
        key_list_->item(i, 0)->setCheckState(Qt::Checked);
      }
    }
  }
}

void KeyTable::UncheckALL() const {
  for (int i = 0; i < key_list_->rowCount(); i++) {
    key_list_->item(i, 0)->setCheckState(Qt::Unchecked);
  }
}

void KeyTable::CheckALL() const {
  for (int i = 0; i < key_list_->rowCount(); i++) {
    key_list_->item(i, 0)->setCheckState(Qt::Checked);
  }
}

void KeyTable::SetMenuAbility(KeyMenuAbility::AbilityType ability) {
  this->ability_ = ability;
}

void KeyTable::SetFilterKeyword(QString keyword) {
  this->keyword_ = std::move(keyword);
}
}  // namespace GpgFrontend::UI
