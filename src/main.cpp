#include "keybd.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    keybd w;
    w.show();
    return a.exec();
}
