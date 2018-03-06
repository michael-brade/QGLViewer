#ifndef QGLVIEWER_H
#define QGLVIEWER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

#include <QMatrix4x4>

#include "camera.h"
#include "gldata.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)


class QGLViewer : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT
public:
  QGLViewer(const CameraConfig& cameraConfig, QWidget *parent = nullptr);
  ~QGLViewer() override;

  QSize minimumSizeHint() const override;
  QSize sizeHint() const override;

  Camera& camera() { return m_camera; }

  void setData(const GLData& data);


public slots:
  void cleanup();

protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int width, int height) override;
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  void initializeGridAndAxes();
  void setupGL();
  void setupVertexAttribs();

  QPoint m_lastPos;

  // draw as triangles
  GLData m_tris;
  QOpenGLVertexArrayObject m_trisVao;
  QOpenGLBuffer m_trisVbo;

  // draw as lines: grid and axes
  GLData m_lines;
  QOpenGLVertexArrayObject m_linesVao;
  QOpenGLBuffer m_linesVbo;

  bool m_drawGrid;
  bool m_drawAxes;

  QOpenGLShaderProgram *m_program;

  Camera m_camera;

  int m_mvpMatrixLoc;
};

#endif
