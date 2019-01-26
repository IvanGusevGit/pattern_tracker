//
// Created by ivan on 10.01.19.
//

#include "main_window.h"
#include "ui_main_window.h"
#include "searcher/searcher.h"

#include <QFileDialog>
#include <QDir>
#include <QProgressBar>
#include <QMetaType>
#include <iostream>
#include <QSet>

Q_DECLARE_METATYPE(QSet<uint64_t>);

//Q_DECLARE_METATYPE(QHash<QString, QSet<uint64_t>>)

namespace TrackStatus {
    QString const IN_QUEUE = "In queue";
    QString const INDEXING = "Indexing...";
    QString const SCANNED = "Scanned";
    QString const STOPPED = "Stopped";
}

main_window::main_window(QWidget *parent)
        : QWidget(parent), ui(new Ui::main_window) {
    ui->setupUi(this);

    qRegisterMetaType<QSet<uint64_t>>("QSet<uint64_t>>");
    qRegisterMetaType<QHash<QString, QSet<uint64_t>>>("QHash<QString, QSet<uint64_t>>");
    init_ui_components();

    connect(ui->selectDirectoryButton, SIGNAL(clicked(bool)), this, SLOT(select_directory()));
    connect(ui->addToTrackingButton, SIGNAL(clicked(bool)), this, SLOT(add_to_tracking()));
    connect(ui->requestArea, SIGNAL(textEdited(const QString &)), this, SLOT(check_request_area()));
    connect(ui->removeDirectoryButton, SIGNAL(clicked(bool)), this, SLOT(remove_directory_from_tracking()));
    connect(ui->stopIndexingButton, SIGNAL(clicked(bool)), this, SLOT(stop_indexing()));
    connect(ui->continueIndexingButton, SIGNAL(clicked(bool)), this, SLOT(continue_indexing()));
    connect(&systemWatcher, &QFileSystemWatcher::directoryChanged, this, &main_window::update_directory);

}

void main_window::init_ui_components() {
    ui->selectedDirectoriesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void main_window::select_directory() {
    QString directory_path = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                               QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->pathArea->setText(directory_path);
}

void main_window::add_to_tracking() {
    QString directoryPath = ui->pathArea->text();
    if (directoryPath.isEmpty()) {
        return;
    }
    ui->pathArea->clear();
    if (!QDir(directoryPath).exists()) {
        ui->notification->setText("Directory does not exist");
        return;
    }

    for (int32_t i = 0; i < ui->selectedDirectoriesView->rowCount(); i++) {
        QString currentDirectoryPath = ui->selectedDirectoriesView->item(i, 0)->text();
        if (currentDirectoryPath.indexOf(directoryPath) != -1 || directoryPath.indexOf(currentDirectoryPath) != -1) {
            ui->notification->setText("Directory intersects with another directories in tracking list");
            return;
        }
    }
    size_t newItemIndex = ui->selectedDirectoriesView->rowCount();
    ui->selectedDirectoriesView->insertRow(ui->selectedDirectoriesView->rowCount());

    ui->selectedDirectoriesView->setItem(newItemIndex, 0, new QTableWidgetItem);
    ui->selectedDirectoriesView->item(newItemIndex, 0)->setText(directoryPath);
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    ui->selectedDirectoriesView->setCellWidget(ui->selectedDirectoriesView->rowCount() - 1, 1, progressBar);
    ui->selectedDirectoriesView->setItem(newItemIndex, 2, new QTableWidgetItem);
    ui->selectedDirectoriesView->item(newItemIndex, 2)->setText(TrackStatus::IN_QUEUE);

    ui->selectedDirectoriesView->item(newItemIndex, 2)->setFlags(ui->selectedDirectoriesView->item(newItemIndex, 2)->flags() ^ Qt::ItemIsSelectable ^ Qt::ItemIsEditable);

    add_to_queue(directoryPath);
}

void main_window::start_indexing(QString const &path) {
    set_status(path, TrackStatus::INDEXING);
    systemWatcher.addPath(path);
    emit scan_directory_signal(path);
}

void main_window::increase_status_bar(qint64 value) {
    uint32_t row = find_row(current_scanner_directory);
    if (row == -1) {
        return;
    }
    QProgressBar* progressBar = (QProgressBar*)ui->selectedDirectoriesView->cellWidget(row, 1);
    progressBar->setValue(progressBar->value() + value);
}

uint32_t main_window::find_row(QString const &path) {
    for (size_t i = 0; i < ui->selectedDirectoriesView->rowCount(); i++) {
        if (ui->selectedDirectoriesView->item(i, 0)->text() == path) {
            return i;
        }
    }
    return -1;
}

void main_window::set_status_bar_range(qint64 range) {
    uint32_t row = find_row(current_scanner_directory);
    if (row == -1) {
        return;
    }
    QProgressBar* progressBar = (QProgressBar*)ui->selectedDirectoriesView->cellWidget(row, 1);
    progressBar->setRange(0, range);
    progressBar->setValue(0);
}

void main_window::add_trigram_data(QHash<QString, QSet<uint64_t>> const &files_data) {
    if (!directories_data.contains(current_scanner_directory)) {
        directories_data[current_scanner_directory] = QSet<QString>();
    }
    QHashIterator<QString, QSet<uint64_t>> file_it(files_data);
    while (file_it.hasNext()) {
        file_it.next();
        directories_data[current_scanner_directory].insert(file_it.key());
        this->files_data[file_it.key()] = file_it.value();
    }
}

void main_window::add_to_queue(QString const &path) {
    queue.push_back(path);
    try_to_launch_directory_scanner();
}

void main_window::try_to_launch_directory_scanner() {
    (std::cout << files_data.size() << '\n').flush();
    if (directory_scanner_thread == nullptr && !queue.empty()) {
        QString nextDirectory = queue.front();
        current_scanner_directory = nextDirectory;
        queue.pop_front();

        directory_scanner_thread = new QThread;
        auto scanner = new directory_scanner(&files_data);
        scanner->moveToThread(directory_scanner_thread);

        connect(this, &main_window::scan_directory_signal, scanner, &directory_scanner::set_directory_path);
        connect(this, &main_window::stop_directory_scanning, scanner, &directory_scanner::stop_scanning);
        connect(scanner, &directory_scanner::range_signal, this, &main_window::set_status_bar_range);
        connect(scanner, &directory_scanner::file_scanned, this, &main_window::increase_status_bar);
        connect(scanner, &directory_scanner::found_trigrams_signal, this, &main_window::add_trigram_data);
        connect(scanner, &directory_scanner::finished_scanning, this, &main_window::reload_directory_scanner);
        connect(scanner, &directory_scanner::finished_scanning, scanner, &QObject::deleteLater);
        connect(directory_scanner_thread, &QThread::finished, directory_scanner_thread, &QThread::deleteLater);
        directory_scanner_thread->start();

        start_indexing(nextDirectory);
    }
}

void main_window::reload_directory_scanner() {
    start_directory_monitoring(current_scanner_directory);
    int32_t row = find_row(current_scanner_directory);
    bool stopped;
    stopped = (row == -1) || ((QProgressBar*)ui->selectedDirectoriesView->cellWidget(row, 1))->text() != "100%";
    if (!stopped) {
        set_status(current_scanner_directory, TrackStatus::SCANNED);
        directory_scanner_thread = nullptr;
        current_scanner_directory = "";
        try_to_launch_directory_scanner();
    }
}

void main_window::check_request_area() {
    QString text = ui->requestArea->text();
    if (text.isEmpty()) {
        ui->foundPathesArea->clear();
    } else {
        start_search(text);
    }
}

void main_window::start_search(QString const &text) {

    auto searcher_thread = new QThread();
    auto engine = new searcher(&files_data, ++latest_searcher_id, text);
    engine->moveToThread(searcher_thread);

    connect(searcher_thread, SIGNAL(started()), engine, SLOT(start_search()));
    connect(engine, &searcher::found_signal, this, &main_window::add_found_path);
    connect(this, &main_window::stop_search_pattern_signal, engine, &searcher::stop_search);
    connect(engine, &searcher::search_finished, engine, &QObject::deleteLater);
    connect(searcher_thread, &QThread::finished, this, &main_window::decrement_search_counter);
    connect(searcher_thread, &QThread::finished, searcher_thread, &QThread::deleteLater);

    ui->foundPathesArea->clear();
    searcher_thread->start();
}

void main_window::add_found_path(size_t id, QString const &path) {
    if (id == latest_searcher_id) {
        ui->foundPathesArea->addItem(path);
    }
}


void main_window::interrupt_search() {
    emit stop_search_pattern_signal();
}

void main_window::decrement_search_counter() {
    running_searchers--;
}


main_window::~main_window() {
    interrupt_search();
    if (directory_scanner_thread != nullptr) {
        emit stop_directory_scanning(current_scanner_directory);
    }
    while (running_searchers > 0 && running_updaters > 0 && directory_scanner_thread != nullptr);
}

void main_window::set_status(QString const &path, QString status) {
    size_t row = find_row(path);
    if (row == -1) {
        return;
    }
    ui->selectedDirectoriesView->item(row, 2)->setText(status);
}

void main_window::remove_directory_from_tracking() {
    auto cells = ui->selectedDirectoriesView->selectedItems();
    bool scanner_stop = false;
    for (size_t i = 0; i < cells.size(); i++) {
        QString path_to_remove = cells.at(i)->text();
        systemWatcher.removePath(path_to_remove);
        if (path_to_remove == current_scanner_directory) {
            emit stop_directory_scanning(path_to_remove);
            scanner_stop = true;
        } else {
            for (size_t i = 0; i < queue.size(); i++) {
                QString current_path = queue.front();
                queue.pop_front();
                if (current_path != path_to_remove) {
                    queue.push_back(current_path);
                }
            }
        }
        ui->selectedDirectoriesView->removeRow(cells.at(i)->row());
        remove_directory_trigrams(path_to_remove);
        stop_directory_monitoring(path_to_remove);
        if (scanner_stop) {
            directory_scanner_thread = nullptr;
            try_to_launch_directory_scanner();
        }
    }
}

void main_window::remove_directory_trigrams(QString const &path) {
    foreach (QString const &file_path, directories_data[path]) {
        files_data.remove(file_path);
    }
    directories_data.remove(path);
}

void main_window::stop_indexing() {
    auto cells = ui->selectedDirectoriesView->selectedItems();
    bool scanner_stop = false;
    for (size_t i = 0; i < cells.size(); i++) {
        QString path_to_remove = cells.at(i)->text();
        QString status = (ui->selectedDirectoriesView->item(cells.at(i)->row(), 2))->text();
        if (status == TrackStatus::SCANNED) {
            continue;
        }
        if (path_to_remove == current_scanner_directory) {
            emit stop_directory_scanning(path_to_remove);
            set_status(path_to_remove, TrackStatus::STOPPED);
            scanner_stop = true;
        } else {
            for (size_t i = 0; i < queue.size(); i++) {
                QString current_path = queue.front();
                queue.pop_front();
                if (current_path != path_to_remove) {
                    queue.push_back(current_path);
                } else {
                    set_status(current_path, TrackStatus::STOPPED);
                }
            }
        }
        if (scanner_stop) {
            directory_scanner_thread = nullptr;
            try_to_launch_directory_scanner();
        }
    }
}

void main_window::continue_indexing() {
    auto cells = ui->selectedDirectoriesView->selectedItems();
    for (size_t i = 0; i < cells.size(); i++) {
        QString path = cells.at(i)->text();
        int32_t row = cells.at(i)->row();
        if (ui->selectedDirectoriesView->item(row, 2)->text() == TrackStatus::STOPPED) {
            set_status(path, TrackStatus::IN_QUEUE);
            add_to_queue(path);
        }
    }
}

void main_window::update_directory(QString const &path) {
    QString parent_path = find_parent_directory(path);
    if (parent_path == current_scanner_directory || queue.contains(parent_path)) {
        return;
    }
    remove_directory_trigrams(parent_path);
    set_status(parent_path, TrackStatus::IN_QUEUE);
    (std::cout << "CHANGE DETECTED " + path.toStdString() << '\n').flush();
    add_to_queue(parent_path);

}

QString main_window::find_parent_directory(QString const &path) {
    for (size_t i = 0; i < ui->selectedDirectoriesView->rowCount(); i++) {
        if (path.indexOf(ui->selectedDirectoriesView->item(i, 0)->text()) == 0) {
            return ui->selectedDirectoriesView->item(i, 0)->text();
        }
    }
    return "";
}

QStringList main_window::get_sub_folders_list(QString folder)
{
    QDir dir(folder);
    dir.setFilter(QDir::Dirs);

    QFileInfoList list = dir.entryInfoList();
    QStringList dirList;

    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        dirList << fileInfo.filePath();
    }

    return dirList;
}

void main_window::start_directory_monitoring(QString const &directory_path) {
    QStringList subfolders = get_sub_folders_list(directory_path);
    subfolders_data[directory_path] = QSet<QString>();
    for (size_t i = 0; i < subfolders.size(); i++) {
        subfolders_data[directory_path].insert(subfolders.at(i));
        systemWatcher.addPath(subfolders.at(i));
    }
}

void main_window::stop_directory_monitoring(QString const &directory_path) {
    QSetIterator<QString> subfolder_it(subfolders_data[directory_path]);
    while (subfolder_it.hasNext()) {
        systemWatcher.removePath(subfolder_it.next());
    }
    subfolders_data.remove(directory_path);
}
