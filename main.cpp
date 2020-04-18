#include "qglviewer.h"
#include "camera.h"

#include <QApplication>


int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  QCoreApplication::setApplicationName("Qt GL Viewer Example");
  QCoreApplication::setApplicationVersion("1.0.0");

  QGLViewer viewer;

  // GL camera config
  Camera *camera = viewer.camera();

  CameraConfig config;
  config.c_mode = CameraMode::Target;
  config.p_mode = ProjectionMode::Perspective;
  config.fov = 45;
  config.nearPlane = 1;
  config.farPlane = 4000.0f;

  // initialTranslation in world coords
  config.initialTranslation = QVector3D(900, 200, 100);

  // how to interpret world coordinates to directions - Z is up
  config.WorldForward = QVector3D(-1, 0, 0);
  config.WorldRight   = QVector3D(0, 1, 0);
  config.WorldUp      = QVector3D(0, 0, 1);

  camera->setConfig(config);

  viewer.show();

  return app.exec();
}