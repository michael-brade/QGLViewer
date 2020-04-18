#include "qglviewer.h"

#include <QMouseEvent>
#include <QOpenGLShaderProgram>

#include <cmath>
#include <iostream>


QGLViewer::QGLViewer(QWidget *parent)
  : QOpenGLWidget(parent),
    m_drawGrid(true),
    m_drawAxes(true),
    m_program(nullptr),
    m_camera()
{
  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setSamples(8);
  setFormat(format);
}

QGLViewer::~QGLViewer() {
  cleanup();
}


QSize QGLViewer::minimumSizeHint() const {
  return QSize(50, 50);
}

QSize QGLViewer::sizeHint() const {
  return QSize(400, 400);
}


void QGLViewer::setData(const GLData& data) {
  m_tris = data;
  setupGL();
  update();
}

void QGLViewer::cleanup() {
  if (m_program == nullptr)
    return;

  makeCurrent();
  m_trisVbo.destroy();
  m_linesVbo.destroy();
  delete m_program;
  m_program = nullptr;
  doneCurrent();
}



static const char *vertexShaderSource = R"(
  attribute vec3 vertex;
  attribute vec3 color;

  uniform mat4 mvpMatrix;

  varying highp vec3 triangle;

  void main(void) {
    triangle = color;
    gl_Position = mvpMatrix * vec4(vertex, 1.0);
  }
)";

static const char *fragmentShaderSource = R"(
  varying highp vec3 triangle;

  void main() {
    gl_FragColor = vec4(triangle, 0.5);
  }
)";


void QGLViewer::initializeGL() {
  // context() and QOpenGLContext::currentContext() are equivalent when called from initializeGL or paintGL.
  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &QGLViewer::cleanup);

  initializeOpenGLFunctions();

  glClearColor(0, 0, 0, 1);
  glDepthFunc(GL_LESS);

  m_program = new QOpenGLShaderProgram;
  m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
  m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
  m_program->bindAttributeLocation("vertex", 0);
  m_program->bindAttributeLocation("color", 1);

  if (!m_program->link())
    std::cerr << "ERROR: failed to link: " << m_program->log().toStdString();

  m_mvpMatrixLoc = m_program->uniformLocation("mvpMatrix");

  // Create a vertex array object. In OpenGL ES 2.0 and OpenGL 2.x
  // implementations this is optional and support may not be present
  // at all. Nonetheless the below code works in all cases and makes
  // sure there is a VAO when one is needed.
  if (!m_trisVao.create() || !m_linesVao.create())
    std::cerr << "ERROR: faild to create vertex array object" << std::endl;

  if (!m_trisVbo.create() || !m_linesVbo.create())
    std::cerr << "ERROR: failed to create vertex buffer object" << std::endl;

  initializeGridAndAxes();
  setupGL();

  m_camera.reset();
}

void QGLViewer::initializeGridAndAxes() {
  // setup grid

  QVector3D color = QVector3D(0.7f, 0.7f, 0.7f);
  for (int x = -2000; x <= 2000; x += 100) {
    for (int y = -2000; y <= 2000; y += 100) {
      // parallel to x
      m_lines.addVertex(QVector3D(-x, y, 0), color);
      m_lines.addVertex(QVector3D(x, y, 0), color);

      // parallel to y
      m_lines.addVertex(QVector3D(x, -y, 0), color);
      m_lines.addVertex(QVector3D(x, y, 0), color);
    }
  }

  // setup coordinate axes

  const float length = 250.0f;
  const float arrSize = 10;

  // x (red)
  color = QVector3D(1,0,0);
  m_lines.addVertex(QVector3D(-length, 0, 0), color);
  m_lines.addVertex(QVector3D(length, 0, 0), color);

  // arrow
  m_lines.addVertex(QVector3D(length, 0, 0), color);
  m_lines.addVertex(QVector3D(length-arrSize, arrSize/2, 0), color);

  m_lines.addVertex(QVector3D(length, 0, 0), color);
  m_lines.addVertex(QVector3D(length-arrSize, -arrSize/2, 0), color);

  // y (green)
  color = QVector3D(0,1,0);
  m_lines.addVertex(QVector3D(0, -length, 0), color);
  m_lines.addVertex(QVector3D(0, length, 0), color);

  // arrow
  m_lines.addVertex(QVector3D(0, length, 0), color);
  m_lines.addVertex(QVector3D(arrSize/2, length-arrSize, 0), color);

  m_lines.addVertex(QVector3D(0, length, 0), color);
  m_lines.addVertex(QVector3D(-arrSize/2, length-arrSize, 0), color);

  // z (blue)
  color = QVector3D(0,0,1);
  m_lines.addVertex(QVector3D(0, 0, -length), color);
  m_lines.addVertex(QVector3D(0, 0, length), color);

  // arrow
  m_lines.addVertex(QVector3D(0, 0, length), color);
  m_lines.addVertex(QVector3D(arrSize/2, 0, length-arrSize), color);

  m_lines.addVertex(QVector3D(0, 0, length), color);
  m_lines.addVertex(QVector3D(-arrSize/2, 0, length-arrSize), color);
}



void QGLViewer::setupGL() {
  m_trisVao.bind();
  m_program->bind();

  // Setup our vertex buffer objects.

  // scene objects are in data VBO
  m_trisVbo.bind();
  m_trisVbo.allocate(m_tris.constData(), m_tris.count() * sizeof(GLfloat));

  // Store the vertex attribute bindings for the program.
  setupVertexAttribs();
  m_trisVbo.release();

  m_trisVao.release();


  m_linesVao.bind();
  m_linesVbo.bind();
  m_linesVbo.allocate(m_lines.constData(), m_lines.count() * sizeof(GLfloat));
  setupVertexAttribs();
  m_linesVbo.release();
  m_linesVao.release();

  m_program->release();
}

void QGLViewer::setupVertexAttribs() {
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  // 3 floats for first group of attributes (triangle pos), then 3 floats for second group (color)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), nullptr);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
}

void QGLViewer::paintGL() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_CULL_FACE);

  m_program->bind();
  m_program->setUniformValue(m_mvpMatrixLoc, m_camera.toMatrix());


  /* It doesn't matter if the vertex attributes are all from one buffer or multiple buffers,
   * and we don't need to bind any particular vertex buffer when drawing; all the glDraw* functions
   * care about is which vertex attribute arrays are enabled.
   */

  m_trisVao.bind();
  // render as wireframe
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawArrays(GL_TRIANGLES, 0, m_tris.vertexCount());
  m_trisVao.release();


  if (m_drawGrid) {
    glLineWidth(0.5f);
    m_linesVao.bind();
    glDrawArrays(GL_LINES, 0, m_lines.vertexCount() - 18);
    m_linesVao.release();
  }

  if (m_drawAxes) {
    glLineWidth(3);
    m_linesVao.bind();
    glDrawArrays(GL_LINES, m_lines.vertexCount() - 18, m_lines.vertexCount());
    m_linesVao.release();
  }

  m_program->release();
}

void QGLViewer::resizeGL(int w, int h) {
  m_camera.setAspectRatio(GLfloat(w) / h);
}

void QGLViewer::keyPressEvent(QKeyEvent *event) {
  switch (event->key()) {
  case Qt::Key_A:
    m_drawAxes = !m_drawAxes;
    break;
  case Qt::Key_G:
    m_drawGrid = !m_drawGrid;
    break;

  case Qt::Key_0:
    m_camera.reset();
    break;
  case Qt::Key_P:
    m_camera.setProjectionMode(ProjectionMode::Perspective);
    break;
  case Qt::Key_O:
    m_camera.setProjectionMode(ProjectionMode::Orthographic);
    break;
  case Qt::Key_F:
    m_camera.setCameraMode(CameraMode::Free);
    break;
  case Qt::Key_T:
    m_camera.setCameraMode(CameraMode::Target);
    break;
  case Qt::Key_L:           // log current camera data
    qDebug() << m_camera;
    break;
  }
  update();
}


void QGLViewer::mousePressEvent(QMouseEvent *event) {
  m_lastPos = event->pos();
}

void QGLViewer::mouseMoveEvent(QMouseEvent *event) {
  float dx = event->x() - m_lastPos.x();
  float dy = event->y() - m_lastPos.y();

  QCursor::setPos(mapToGlobal(m_lastPos));

  if (event->modifiers() & Qt::ShiftModifier) {
    dx /= 4;
    dy /= 4;
  }

  if (event->buttons() & Qt::LeftButton) {
    if (m_camera.cameraMode() == CameraMode::Free)
      m_camera.rotate(-0.2f * dx, m_camera.upVector());
    else if (m_camera.upsideDown())
      m_camera.rotate(-0.2f * dx, -m_camera.worldUpVector()); // if the up vector actually points down, reverse rotation
    else
      m_camera.rotate(-0.2f * dx, m_camera.worldUpVector());

    m_camera.rotate(-0.2f * dy, m_camera.rightVector());
  } else if (event->buttons() & Qt::RightButton) {
    if (m_camera.cameraMode() == CameraMode::Free) {
      dx *= -1;
    }

    m_camera.rotate(-0.2f * dx, m_camera.forwardVector());
    m_camera.rotate(-0.2f * dy, m_camera.rightVector());
  } else if (event->buttons() & Qt::MiddleButton) {
    if (m_camera.cameraMode() == CameraMode::Free) {
      dx *= -1;
      dy *= -1;
    }

    m_camera.translate(-dx * m_camera.rightVector());
    m_camera.translate( dy * m_camera.upVector());
  }

  update();
}

void QGLViewer::wheelEvent(QWheelEvent *event) {
  QPoint numDeg = event->angleDelta();

  if (numDeg.isNull())
    return;

  float factor = 150;

  if (event->modifiers() & Qt::ShiftModifier)
    factor /= 10;

  if (numDeg.y() < 0)
    factor = -factor;

  m_camera.translate(factor * m_camera.forwardVector());

  event->accept();
  update();
}
