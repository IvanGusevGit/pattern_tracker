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
    try {
        for (QString const &file_path : roots[id]) {
            checkInterruption();
            get_file_trigrams(file_path);
            emit file_scanned(1);
        }
    } catch (std::exception &e) {
        //stopped
    }
    emit finished_scanning();
}

void file_scanner::get_file_trigrams(QString const &path) {
    QSet<uint64_t> result;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        QString buffer;
        while (true) {
            checkInterruption();
            buffer.append(stream.read(BUFFER_SIZE));
            if (buffer.size() < 3) {
                break;
            }
            for (int i = 0; i < buffer.size() - 2; i++) {
                uint64_t current_trigram = buffer[i].unicode();
                for (int j = i + 1; j < i + 3; j++) {
                    current_trigram = (current_trigram << 16) + buffer[j].unicode();
                }
                result.insert(current_trigram);
                if (result.size() > 20000) {
                    result.clear();
                    break;
                }
            }
            buffer = buffer.mid(buffer.size() - 2, 2);
        }
        file.close();
    }

    if (!result.isEmpty()) {
        emit found_trigrams(path, result);
    }
}

void file_scanner::checkInterruption() {
    if (QThread::currentThread()->isInterruptionRequested()) {
        throw std::exception();
    }
}




