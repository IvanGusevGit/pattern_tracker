//
// Created by ivan on 12.01.19.
//

#include <QHashIterator>
#include <QtCore/QSet>
#include "searcher.h"
#include <QFile>
#include <QtCore/QTextStream>
#include <iostream>
#include <cmath>

searcher::searcher(QHash<QString, QSet<uint64_t>> *files_data, qint64 id, QString const &request) : files_data(files_data), id(id), request(request) {}

void searcher::start_search() {
    stop_flag = false;
    QSet<uint64_t> request_trigrams = calc_string_trigrams(request.toStdString());
    QHashIterator<QString, QSet<uint64_t>> i(*files_data);
    size_t matches = 0;
    while (i.hasNext()) {
        if (stop_flag) {
            break;
        }
        i.next();
        if (i.value().contains(request_trigrams) || request_trigrams.size() == 0) {
            if (check_file_for_substring(i.key(), request)) {
                matches++;
                emit found_signal(id, i.key());
            }
        }
    }
    emit search_finished(matches);
}

QSet<uint64_t> searcher::calc_string_trigrams(std::string text) {
    QSet<uint64_t> trigrams;
    if (text.size() < 3) {
        return trigrams;
    }
    for (ptrdiff_t i = 0; i <= static_cast<ptrdiff_t>(text.size()) - 3; ++i) {
        uint64_t hash = 0;
        for (int j = 0; j < 3; ++j) {
            hash = (hash << 16);
            hash += reinterpret_cast<unsigned char &>(text[i + j]);
        }
        trigrams.insert(hash);
    }
    return trigrams;
}

bool searcher::check_file_for_substring(QString const &path, QString const &request) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QString buffer;
        bool found = false;
        while (true) {
            buffer.append(stream.read(BUFFER_SIZE));
            if (stop_flag) {
                return false;
            }
            if (buffer.size() < request.size()) {
                break;
            }
            if (buffer.indexOf(request) >= 0) {
                found = true;
                break;
            }
            buffer = buffer.mid(buffer.size() - request.size() + 1, request.size() - 1);
        }
        file.close();
        return found;
    }
    return false;
}

void searcher::stop_search() {
    stop_flag = true;
}


