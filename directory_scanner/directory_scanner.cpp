//
// Created by ivan on 11.01.19.
//

#include <QtCore/QVector>
#include <QtCore/QDirIterator>
#include "directory_scanner.h"

#include <thread>
#include <iostream>

Q_DECLARE_METATYPE(std::vector<std::vector<QString>>)

directory_scanner::directory_scanner(QHash<QString, QSet<uint64_t >>* storage) : file_storage(storage) {
    qRegisterMetaType<std::vector<std::vector<QString>>>("std::vector<std::vector<QString>>");
    for (size_t i = 0; i < SCANNERS_NUMBER; i++) {
        threads[i] = new QThread;
        auto *scanner = new file_scanner(i);
        scanner->moveToThread(threads[i]);

        connect(this, &directory_scanner::file_groups_signal, scanner, &file_scanner::start_scanning);
        connect(scanner, &file_scanner::file_scanned, this, &directory_scanner::emit_scanned_signal);
        connect(scanner, &file_scanner::found_trigrams, this, &directory_scanner::emit_found_trigrams_signal);
        connect(scanner, &file_scanner::finished_scanning, this, &directory_scanner::increment_finished_threads_counter);


        threads[i]->start();
    }
}

void directory_scanner::start_scanning() {
    finished_treads_counter = 0;
    std::pair<uint64_t , std::vector<QString>> files_data = find_files();
    emit range_signal(files_data.first);
    emit_scanned_signal(files_data.first - files_data.second.size());
    std::vector<std::vector<QString>> groups(SCANNERS_NUMBER);
    for (size_t i = 0; i < files_data.second.size(); i++) {
        groups[i % SCANNERS_NUMBER].push_back(files_data.second[i]);
    }
    emit file_groups_signal(groups);
}

std::pair<uint64_t , std::vector<QString>> directory_scanner::find_files() {
    uint64_t total = 0;
    std::vector<QString> files;
    QDirIterator it(directory_path, QDir::Hidden | QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        files.push_back(it.next());
        total++;
        if (it.fileInfo().isSymLink()) {
            files.pop_back();
            total--;
        }
        if (file_storage->contains(it.path())) {
            files.pop_back();
        }
    }
    return std::make_pair(total, files);
}

void directory_scanner::emit_scanned_signal(qint64 t) {
    emit file_scanned(t);
}

void directory_scanner::emit_found_trigrams_signal(QString const &path, QSet<uint64_t> const &trigrams) {
    emit found_trigrams_signal(path, trigrams);
}

void directory_scanner::increment_finished_threads_counter() {
    finished_treads_counter++;
    if (finished_treads_counter == SCANNERS_NUMBER) {
        for (size_t i = 0; i < SCANNERS_NUMBER; i++) {
            threads[i] = nullptr;
        }
        emit finished_scanning();
    }
}

void directory_scanner::set_directory_path(QString const &path) {
    directory_path = path;
    start_scanning();
}

directory_scanner::~directory_scanner() {
    stop_scanning(directory_path);
}

void directory_scanner::stop_scanning(QString const &path) {
    for (size_t i = 0; i < SCANNERS_NUMBER; i++) {
        if (threads[i] != nullptr) {
            threads[i]->requestInterruption();
        }
    }
}

