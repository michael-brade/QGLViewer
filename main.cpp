#include "qglviewer.h"

#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("Qt GL Viewer Example");
    QCoreApplication::setApplicationVersion(1.0.0);

    QGLViewer viewer;

    viewer.show();

    return app.exec();
}