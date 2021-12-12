/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
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

#include "ui/widgets/FilePage.h"

#include <boost/filesystem.hpp>
#include <codecvt>
#include <iostream>
#include <locale>
#include <string>

#include "ui/MainWindow.h"
#include "ui/SignalStation.h"

namespace GpgFrontend::UI {

FilePage::FilePage(QWidget* parent) : QWidget(parent), Ui_FilePage() {
  setupUi(this);

  firstParent = parent;

  dirModel = new QFileSystemModel();
  dirModel->setRootPath(QDir::currentPath());
  dirModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

  fileTreeView->setModel(dirModel);
  fileTreeView->setColumnWidth(0, 320);
  fileTreeView->sortByColumn(0, Qt::AscendingOrder);
  mPath = boost::filesystem::path(dirModel->rootPath().toStdString());

  createPopupMenu();

  connect(upPathButton, &QPushButton::clicked, this, &FilePage::slotUpLevel);
  connect(refreshButton, &QPushButton::clicked, this, &FilePage::slotGoPath);
  optionsButton->setMenu(optionPopUpMenu);

  pathEdit->setText(dirModel->rootPath());

  pathEditCompleter = new QCompleter(this);
  pathCompleteModel = new QStringListModel();
  pathEditCompleter->setModel(pathCompleteModel);
  pathEditCompleter->setCaseSensitivity(Qt::CaseInsensitive);
  pathEditCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
  pathEdit->setCompleter(pathEditCompleter);

  connect(fileTreeView, &QTreeView::clicked, this,
          &FilePage::fileTreeViewItemClicked);
  connect(fileTreeView, &QTreeView::doubleClicked, this,
          &FilePage::fileTreeViewItemDoubleClicked);
  connect(fileTreeView, &QTreeView::customContextMenuRequested, this,
          &FilePage::onCustomContextMenu);

  connect(pathEdit, &QLineEdit::textChanged, [=]() {
    auto path = pathEdit->text();
    auto dir = QDir(path);
    if (path.endsWith("/") && dir.isReadable()) {
      auto dir_list = dir.entryInfoList(QDir::AllEntries);
      QStringList paths;
      for (int i = 1; i < dir_list.size(); i++) {
        const auto file_path = dir_list.at(i).filePath();
        const auto file_name = dir_list.at(i).fileName();
        if (file_name == "." || file_name == "..") continue;
        paths.append(file_path);
      }
      pathCompleteModel->setStringList(paths);
    }
  });

  connect(this, &FilePage::signalRefreshInfoBoard, SignalStation::GetInstance(),
          &SignalStation::signalRefreshInfoBoard);
}

void FilePage::fileTreeViewItemClicked(const QModelIndex& index) {
  selectedPath = boost::filesystem::path(
      dirModel->fileInfo(index).absoluteFilePath().toStdString());
  mPath = selectedPath;
  LOG(INFO) << "selected path" << selectedPath;
}

void FilePage::slotUpLevel() {
  QModelIndex currentRoot = fileTreeView->rootIndex();

  auto utf8_path =
      dirModel->fileInfo(currentRoot).absoluteFilePath().toStdString();

#ifdef WINDOWS
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>> converter;
  std::wstring w_path = converter.from_bytes(utf8_path);
  boost::filesystem::path path_obj(w_path);
#else
  boost::filesystem::path path_obj(utf8_path);
#endif

  mPath = path_obj;
  LOG(INFO) << "get path" << mPath;
  if (mPath.has_parent_path() && !mPath.parent_path().empty()) {
    mPath = mPath.parent_path();
    LOG(INFO) << "parent path" << mPath;
    pathEdit->setText(mPath.string().c_str());
    this->slotGoPath();
  }
}

void FilePage::fileTreeViewItemDoubleClicked(const QModelIndex& index) {
  QFileInfo file_info(dirModel->fileInfo(index).absoluteFilePath());
  if (file_info.isFile()) {
    slotOpenItem();
  } else {
    pathEdit->setText(file_info.filePath());
    slotGoPath();
  }
}

QString FilePage::getSelected() const {
  return QString::fromStdString(selectedPath.string());
}

void FilePage::slotGoPath() {
  const auto path_edit = pathEdit->text().toStdString();

#ifdef WINDOWS
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>> converter;
  std::wstring w_path = converter.from_bytes(path_edit);
  boost::filesystem::path path_obj(w_path);
#else
  boost::filesystem::path path_obj(path_edit);
#endif

  if (mPath.string() != path_edit) mPath = path_obj;
  auto fileInfo = QFileInfo(mPath.string().c_str());
  if (fileInfo.isDir() && fileInfo.isReadable() && fileInfo.isExecutable()) {
    mPath = boost::filesystem::path(fileInfo.filePath().toStdString());
    LOG(INFO) << "set path" << mPath;
    fileTreeView->setRootIndex(dirModel->index(fileInfo.filePath()));
    dirModel->setRootPath(fileInfo.filePath());
    for (int i = 1; i < dirModel->columnCount(); ++i) {
      fileTreeView->resizeColumnToContents(i);
    }
    pathEdit->setText(mPath.generic_path().string().c_str());
  } else {
    QMessageBox::critical(
        this, _("Error"),
        _("The path is not exists, unprivileged or unreachable."));
  }
  emit pathChanged(mPath.string().c_str());
}

void FilePage::createPopupMenu() {
  popUpMenu = new QMenu();

  openItemAct = new QAction(_("Open"), this);
  connect(openItemAct, &QAction::triggered, this, &FilePage::slotOpenItem);
  renameItemAct = new QAction(_("Rename"), this);
  connect(renameItemAct, &QAction::triggered, this, &FilePage::slotRenameItem);
  deleteItemAct = new QAction(_("Delete"), this);
  connect(deleteItemAct, &QAction::triggered, this, &FilePage::slotDeleteItem);
  encryptItemAct = new QAction(_("Encrypt Sign"), this);
  connect(encryptItemAct, &QAction::triggered, this,
          &FilePage::slotEncryptItem);
  decryptItemAct =
      new QAction(QString(_("Decrypt Verify")) + " " + _("(.gpg .asc)"), this);
  connect(decryptItemAct, &QAction::triggered, this,
          &FilePage::slotDecryptItem);
  signItemAct = new QAction(_("Sign"), this);
  connect(signItemAct, &QAction::triggered, this, &FilePage::slotSignItem);
  verifyItemAct =
      new QAction(QString(_("Verify")) + " " + _("(.sig .gpg .asc)"), this);
  connect(verifyItemAct, &QAction::triggered, this, &FilePage::slotVerifyItem);

  hashCalculateAct = new QAction(_("Calculate Hash"), this);
  connect(hashCalculateAct, &QAction::triggered, this,
          &FilePage::slotCalculateHash);

  mkdirAct = new QAction(_("Make New Directory"), this);
  connect(mkdirAct, &QAction::triggered, this, &FilePage::slotMkdir);

  createEmptyFileAct = new QAction(_("Create Empty File"), this);
  connect(createEmptyFileAct, &QAction::triggered, this,
          &FilePage::slotCreateEmptyFile);

  popUpMenu->addAction(openItemAct);
  popUpMenu->addAction(renameItemAct);
  popUpMenu->addAction(deleteItemAct);
  popUpMenu->addSeparator();
  popUpMenu->addAction(encryptItemAct);
  popUpMenu->addAction(decryptItemAct);
  popUpMenu->addAction(signItemAct);
  popUpMenu->addAction(verifyItemAct);
  popUpMenu->addSeparator();
  popUpMenu->addAction(mkdirAct);
  popUpMenu->addAction(createEmptyFileAct);
  popUpMenu->addAction(hashCalculateAct);

  optionPopUpMenu = new QMenu();

  auto showHiddenAct = new QAction(_("Show Hidden File"), this);
  showHiddenAct->setCheckable(true);
  connect(showHiddenAct, &QAction::triggered, this, [&](bool checked) {
    LOG(INFO) << "Set Hidden" << checked;
    if (checked)
      dirModel->setFilter(dirModel->filter() | QDir::Hidden);
    else
      dirModel->setFilter(dirModel->filter() & ~QDir::Hidden);
    dirModel->setRootPath(mPath.string().c_str());
  });
  optionPopUpMenu->addAction(showHiddenAct);

  auto showSystemAct = new QAction(_("Show System File"), this);
  showSystemAct->setCheckable(true);
  connect(showSystemAct, &QAction::triggered, this, [&](bool checked) {
    LOG(INFO) << "Set Hidden" << checked;
    if (checked)
      dirModel->setFilter(dirModel->filter() | QDir::System);
    else
      dirModel->setFilter(dirModel->filter() & ~QDir::System);
    dirModel->setRootPath(mPath.string().c_str());
  });
  optionPopUpMenu->addAction(showSystemAct);
}

void FilePage::onCustomContextMenu(const QPoint& point) {
  QModelIndex index = fileTreeView->indexAt(point);
  selectedPath = boost::filesystem::path(
      dirModel->fileInfo(index).absoluteFilePath().toStdString());
  LOG(INFO) << "right click" << selectedPath;
  if (index.isValid()) {
    openItemAct->setEnabled(true);
    renameItemAct->setEnabled(true);
    deleteItemAct->setEnabled(true);

    QFileInfo info(QString::fromStdString(selectedPath.string()));
    encryptItemAct->setEnabled(
        info.isFile() && (info.suffix() != "gpg" && info.suffix() != "sig"));
    encryptItemAct->setEnabled(
        info.isFile() && (info.suffix() != "gpg" && info.suffix() != "sig"));
    decryptItemAct->setEnabled(info.isFile() && info.suffix() == "gpg");
    signItemAct->setEnabled(info.isFile() &&
                            (info.suffix() != "gpg" && info.suffix() != "sig"));
    verifyItemAct->setEnabled(
        info.isFile() && (info.suffix() == "sig" || info.suffix() == "gpg"));
    hashCalculateAct->setEnabled(info.isFile() && info.isReadable());
  } else {
    openItemAct->setEnabled(false);
    renameItemAct->setEnabled(false);
    deleteItemAct->setEnabled(false);

    encryptItemAct->setEnabled(false);
    encryptItemAct->setEnabled(false);
    decryptItemAct->setEnabled(false);
    signItemAct->setEnabled(false);
    verifyItemAct->setEnabled(false);
    hashCalculateAct->setEnabled(false);
  }
  popUpMenu->exec(fileTreeView->viewport()->mapToGlobal(point));
}

void FilePage::slotOpenItem() {
  QFileInfo info(QString::fromStdString(selectedPath.string()));
  if (info.isDir()) {
    if (info.isReadable() && info.isExecutable()) {
      const auto file_path = info.filePath().toStdString();
      LOG(INFO) << "set path" << file_path;
      pathEdit->setText(info.filePath());
      slotGoPath();
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("The directory is unprivileged or unreachable."));
    }
  } else {
    if (info.isReadable()) {
      auto mainWindow = qobject_cast<MainWindow*>(firstParent);
      LOG(INFO) << "open item" << selectedPath;
      auto qt_path = QString::fromStdString(selectedPath.string());
      if (mainWindow != nullptr) mainWindow->slotOpenFile(qt_path);
    } else {
      QMessageBox::critical(this, _("Error"),
                            _("The file is unprivileged or unreachable."));
    }
  }
}

void FilePage::slotRenameItem() {
  auto new_name_path = selectedPath, old_name_path = selectedPath;
  auto old_name = old_name_path.filename();
  new_name_path = new_name_path.remove_filename();

  bool ok;
  auto text =
      QInputDialog::getText(this, _("Rename"), _("New Filename"),
                            QLineEdit::Normal, old_name.string().c_str(), &ok);
  if (ok && !text.isEmpty()) {
    try {
      new_name_path /= text.toStdString();
      LOG(INFO) << "new name path" << new_name_path;
      boost::filesystem::rename(old_name_path, new_name_path);
      // refresh
      this->slotGoPath();
    } catch (...) {
      LOG(ERROR) << "rename error" << new_name_path;
      QMessageBox::critical(this, _("Error"),
                            _("Unable to rename the file or folder."));
    }
  }
}

void FilePage::slotDeleteItem() {
  QModelIndex index = fileTreeView->currentIndex();
  QVariant data = fileTreeView->model()->data(index);

  auto ret = QMessageBox::warning(this, _("Warning"),
                                  _("Are you sure you want to delete it?"),
                                  QMessageBox::Ok | QMessageBox::Cancel);

  if (ret == QMessageBox::Cancel) return;

  LOG(INFO) << "Delete Item" << data.toString().toStdString();

  if (!dirModel->remove(index)) {
    QMessageBox::critical(this, _("Error"),
                          _("Unable to delete the file or folder."));
  }
}

void FilePage::slotEncryptItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->slotFileEncryptSign();
}

void FilePage::slotDecryptItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->slotFileDecryptVerify();
}

void FilePage::slotSignItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->slotFileSign();
}

void FilePage::slotVerifyItem() {
  auto mainWindow = qobject_cast<MainWindow*>(firstParent);
  if (mainWindow != nullptr) mainWindow->slotFileVerify();
}

void FilePage::slotCalculateHash() {
  // Returns empty QByteArray() on failure.
  QFileInfo info(QString::fromStdString(selectedPath.string()));

  if (info.isFile() && info.isReadable()) {
    std::stringstream ss;

    ss << "[#] " << _("File Hash Information") << std::endl;
    ss << "    " << _("filename") << _(": ")
       << selectedPath.filename().string().c_str() << std::endl;

    QFile f(info.filePath());
    f.open(QFile::ReadOnly);
    auto buffer = f.readAll();
    LOG(INFO) << "buffer size" << buffer.size();
    f.close();
    if (f.open(QFile::ReadOnly)) {
      auto hash_md5 = QCryptographicHash(QCryptographicHash::Md5);
      // md5
      hash_md5.addData(buffer);
      auto md5 = hash_md5.result().toHex().toStdString();
      LOG(INFO) << "md5" << md5;
      ss << "    "
         << "md5" << _(": ") << md5 << std::endl;

      auto hash_sha1 = QCryptographicHash(QCryptographicHash::Sha1);
      // sha1
      hash_sha1.addData(buffer);
      auto sha1 = hash_sha1.result().toHex().toStdString();
      LOG(INFO) << "sha1" << sha1;
      ss << "    "
         << "sha1" << _(": ") << sha1 << std::endl;

      auto hash_sha256 = QCryptographicHash(QCryptographicHash::Sha256);
      // sha1
      hash_sha256.addData(buffer);
      auto sha256 = hash_sha256.result().toHex().toStdString();
      LOG(INFO) << "sha256" << sha256;
      ss << "    "
         << "sha256" << _(": ") << sha256 << std::endl;

      ss << std::endl;

      emit signalRefreshInfoBoard(ss.str().c_str(),
                                  InfoBoardStatus::INFO_ERROR_OK);
    }
  }
}

void FilePage::slotMkdir() {
  auto index = fileTreeView->rootIndex();

  QString new_dir_name;
  bool ok;
  new_dir_name =
      QInputDialog::getText(this, _("Make New Directory"), _("Directory Name"),
                            QLineEdit::Normal, new_dir_name, &ok);
  if (ok && !new_dir_name.isEmpty()) {
    dirModel->mkdir(index, new_dir_name);
  }
}

void FilePage::slotCreateEmptyFile() {
  auto root_path_str = dirModel->rootPath().toStdString();
#ifdef WINDOWS
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>> converter;
  std::wstring w_path = converter.from_bytes(root_path_str);
  boost::filesystem::path root_path(w_path);
#else
  boost::filesystem::path root_path(root_path_str);
#endif
  QString new_file_name;
  bool ok;
  new_file_name = QInputDialog::getText(this, _("Create Empty File"),
                                        _("Filename (you can given extension)"),
                                        QLineEdit::Normal, new_file_name, &ok);
  if (ok && !new_file_name.isEmpty()) {
    auto file_path = root_path / new_file_name.toStdString();
    QFile new_file(file_path.string().c_str());
    if (!new_file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
      QMessageBox::critical(this, _("Error"), _("Unable to create the file."));
    }
    new_file.close();
  }
}

void FilePage::keyPressEvent(QKeyEvent* event) {
  LOG(INFO) << "Key Press" << event->key();
  if (pathEdit->hasFocus() &&
      (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
    slotGoPath();
  } else if (fileTreeView->currentIndex().isValid()) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
      slotOpenItem();
    else if (event->key() == Qt::Key_Delete ||
             event->key() == Qt::Key_Backspace)
      slotDeleteItem();
  }
}

}  // namespace GpgFrontend::UI
