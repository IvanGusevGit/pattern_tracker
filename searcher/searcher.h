//
// Created by ivan on 12.01.19.
//

#ifndef PATTERN_TRACKER_SEARCHER_H
#define PATTERN_TRACKER_SEARCHER_H


#include <QtCore/QObject>

class searcher : public QObject {
Q_OBJECT
public:
    explicit searcher(QHash<QString, QSet<uint64_t>>* files_data, qint64 id, QString const &request);

    ~searcher() = default;

signals:

    void found_signal(qint64 id, QString const &path);
    void search_finished(qint64);

public slots:

    void start_search();

    void stop_search();

private:

    QSet<uint64_t> calc_string_trigrams(QString const &s);

    bool check_file_for_substring(QString const &path, QString const &request);


    static size_t const BUFFER_SIZE = 1 << 13;

    QHash<QString, QSet<uint64_t>>* files_data;

    QString request;
    bool stop_flag = false;
    qint64 id;
};


#endif //PATTERN_TRACKER_SEARCHER_H
