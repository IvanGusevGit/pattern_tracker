//
// Created by ivan on 10.01.19.
//


#include "main_window.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    main_window w;
    w.show();

    return a.exec();
}


