//
// Created by ivan on 11.01.19.
//

#ifndef PATTERN_TRACKER_DIRECTORY_SCANNER_H
#define PATTERN_TRACKER_DIRECTORY_SCANNER_H

#include "directory_scanner/file_scanner/file_scanner.h"

#include <QtCore/QThread>

#include <vector>

class directory_scanner : public QObject {
Q_OBJECT
public:
    explicit directory_scanner(QHash<QString, QSet<uint64_t>>* storage);

    ~directory_scanner();

signals:

    void range_signal(qint64);

    void file_scanned(qint64);

    void found_trigrams_signal(QHash<QString, QSet<uint64_t>> const &trigrams);

    void finished_scanning();

    void file_groups_signal(std::vector<std::vector<QString>> const &roots);


public slots:

    void start_scanning();

    void emit_scanned_signal(qint64 t);

    void increment_finished_threads_counter(QHash<QString, QSet<uint64_t>> const &trigrams);

    void set_directory_path(QString const &path);

    void stop_scanning(QString const &path);


private:


    std::pair<uint64_t , std::vector<QString>> find_files();


    static size_t const SCANNERS_NUMBER = 3;
    QThread* threads[SCANNERS_NUMBER];
    size_t finished_treads_counter;

    QString directory_path;
    QHash<QString, QSet<uint64_t>>* file_storage;
};


#endif //PATTERN_TRACKER_DIRECTORY_SCANNER_H
