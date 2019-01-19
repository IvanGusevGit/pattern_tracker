//
// Created by ivan on 10.01.19.
//

#ifndef PATTERN_TRACKER_MAIN_WINDOW_H
#define PATTERN_TRACKER_MAIN_WINDOW_H

#include "ui_main_window.h"
#include "directory_scanner/directory_scanner.h"

#include <QWidget>
#include <QScopedPointer>
#include <QQueue>
#include <QFileSystemWatcher>


namespace Ui {
    class main_window;
}

class main_window : public QWidget {
Q_OBJECT

public:
    explicit main_window(QWidget *parent = nullptr);

    ~main_window();

signals:

    void scan_directory_signal(QString const &path);

    void stop_directory_scanning(QString const &path);

    void stop_search_pattern_signal();

    void update_file_signal(std::vector<std::vector<QString>>);

public slots:

    void select_directory();
    void add_to_tracking();
    void remove_directory_from_tracking();

    void set_status_bar_range(qint64 range);
    void increase_status_bar(qint64 value);
    void set_status(QString const &path, QString status);

    void add_trigram_data(QString const &path, QSet<uint64_t> const &trigrams);

    void try_to_launch_directory_scanner();
    void reload_directory_scanner();

    void stop_indexing();
    void continue_indexing();

    void check_request_area();
    void add_found_path(size_t id, QString const &path);
    void decrement_search_counter();

    void update_directory(QString const &path);
    void decrease_running_updaters();
private:

    void init_ui_components();

    void add_to_queue(QString const &path);

    void start_indexing(QString const &path);

    uint32_t find_row(QString const &path);

    void start_search(QString const &path);

    void interrupt_search();

    void remove_directory_trigrams(QString const &path);

    QString find_parent_directory(QString const &path);

    QHash<QString, QSet<QString>> directories_data;
    QHash<QString, QSet<uint64_t>> files_data;

    QScopedPointer<Ui::main_window> ui;

    QQueue<QString> queue;
    QString current_scanner_directory;
    QThread* directory_scanner_thread = nullptr;

    size_t running_searchers = 0;
    qint64 latest_searcher_id = 0;

    QFileSystemWatcher systemWatcher;
    size_t running_updaters = 0;
};


#endif //PATTERN_TRACKER_MAIN_WINDOW_H
