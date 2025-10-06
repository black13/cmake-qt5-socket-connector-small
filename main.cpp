#include <QApplication>
#include "window.h"   // make sure this header exists and defines Window

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // main() stays a pure bootstrapper — no XML, no factory, no I/O here.
    Window window;
    window.show();

    return app.exec();
}
