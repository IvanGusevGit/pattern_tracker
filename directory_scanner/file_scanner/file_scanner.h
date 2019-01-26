//
// Created by ivan on 11.01.19.
//

#ifndef PATTERN_TRACKER_FILE_SCANNER_H
#define PATTERN_TRACKER_FILE_SCANNER_H


#include <QtCore/QThread>
#include <QSet>
#include <unordered_set>
#include <atomic>

class file_scanner : public QObject {
Q_OBJECT
public:
    explicit file_scanner(size_t id);

    ~file_scanner() = default;

signals:

    void file_scanned(qint64);

    void finished_scanning(QHash<QString, QSet<uint64_t>> const &trigrams);

public slots:

    void start_scanning(std::vector<std::vector<QString>> const &roots);

private:

    void get_file_trigrams(QString const &filepath, QHash<QString, QSet<uint64_t>> &trigram_collection);

    void add_string_trigrams(QSet<uint64_t> &trigrams, const char *buffer, qint64 size);

    void checkInterruption();

    static size_t const BUFFER_SIZE = 1 << 13;

    size_t id;
};


#endif //PATTERN_TRACKER_FILE_SCANNER_H
