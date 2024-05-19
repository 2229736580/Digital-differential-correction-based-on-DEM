#include "testCRCS.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    testCRCS w;
    w.show();
    return a.exec();
}
