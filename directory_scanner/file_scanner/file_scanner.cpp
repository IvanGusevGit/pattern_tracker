#include <utility>

//
// Created by ivan on 11.01.19.
//

#include <QtCore/QHash>
#include <QtCore/QFile>
#include <iostream>
#include <QtCore/QTextStream>
#include "file_scanner.h"

#include <unordered_set>

file_scanner::file_scanner(size_t id) : id(id) {}

void file_scanner::start_scanning(std::vector<std::vector<QString>> const &roots) {
    qRegisterMetaType<QHash<QString, QSet<uint64_t>>>("QHash<QString, QSet<uint64_t>>");
    QHash<QString, QSet<uint64_t>> trigram_collection;
    try {
        for (QString const &file_path : roots[id]) {
            checkInterruption();
            get_file_trigrams(file_path, trigram_collection);
            emit file_scanned(1);
        }
    } catch (std::exception &e) {
        //stopped
    }
    (std::cout << "Scanner " + std::to_string(id) + " finished\n").flush();
    emit finished_scanning(trigram_collection);
}

void file_scanner::get_file_trigrams(QString const &filepath, QHash<QString, QSet<uint64_t>> &trigram_collection) {
    QFile file(filepath);
    if (file.open(QIODevice::ReadOnly)) {
        QSet<uint64_t> tr;
        char buf[BUFFER_SIZE + 3 - 1];
        qint64 read = file.read(buf, BUFFER_SIZE) - 3 + 1;
        if (read >= 1)
            do {
                read += 2;
                checkInterruption();
                add_string_trigrams(tr, buf, read);
                if (tr.size() > 20000) {
                    tr.clear();
                    return;
                }
                for (int i = 1; i < 3; ++i) {
                    buf[i - 1] = buf[BUFFER_SIZE - 3 + i];
                }
            } while ((read = file.read(buf + 3 - 1, BUFFER_SIZE)) >= 1);
        trigram_collection.insert(filepath, tr);
        file.close();
    } else {
        std::cout << filepath.toStdString() << "\n";
    }
}

void file_scanner::add_string_trigrams(QSet<uint64_t> &trigrams, const char *buffer, qint64 size) {
    for (ptrdiff_t i = 0; i <= static_cast<ptrdiff_t>(size) - 3; ++i) {
        uint64_t hash = 0;
        for (int j = 0; j < 3; ++j) {
            hash = (hash << 16);
            hash += static_cast<unsigned char>(buffer[i + j]);
        }
        trigrams.insert(hash);
    }
}

void file_scanner::checkInterruption() {
    if (QThread::currentThread()->isInterruptionRequested()) {
        throw std::exception();
    }
}




