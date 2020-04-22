#include "qglviewer.h"
#include "camera.h"

#include <QMouseEvent>
#include <QOpenGLShaderProgram>

#include <cmath>
#include <iostream>


// grid config defaults
GridConfig::GridConfig()
  : minX(-2000), maxX(2000),
    minY(-2000), maxY(2000),
    step(100),
    color(QVector3D(0.7f, 0.7f, 0.7f))
{}

// axes config defaults
AxesConfig::AxesConfig()
  : length(250.0f),
    arrowSize(10.0f),
    colorX(QVector3D(1, 0, 0)),
    colorY(QVector3D(0, 1, 0)),
    colorZ(QVector3D(0, 0, 1))
{}


QGLViewer::QGLViewer(QWidget *parent)
  : QOpenGLWidget(parent),
    m_drawGrid(true),
    m_gridVertexIdx(-1),
    m_gridConfig(),
    m_drawAxes(true),
    m_axesVertexIdx(-1),
    m_axesConfig(),
    m_program(nullptr),
    m_camera(new Camera)
{
  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setSamples(8);
  setFormat(format);
}

QGLViewer::~QGLViewer() {
  cleanup();
  delete m_camera;
  m_camera = nullptr;
}


QSize QGLViewer::minimumSizeHint() const {
  return QSize(50, 50);
}

QSize QGLViewer::sizeHint() const {
  return QSize(400, 400);
}


void QGLViewer::setData(const GLData &data) {
  m_data = data;

  // assumption: data has no grid or axes yet
  m_gridVertexIdx = -1;
  m_axesVertexIdx = -1;

  setupGL();
  update();
}

void QGLViewer::setGridConfig(const GridConfig &grid) {
  m_gridConfig = grid;
  setupGL();
  update();
}

void QGLViewer::setAxesConfig(const AxesConfig &axes) {
  m_axesConfig = axes;
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

  setupGL();

  m_camera->reset();
}


void QGLViewer::initializeGridAndAxes() {
  if (m_gridVertexIdx > -1) {
    // if there were already a grid and axes, delete them before rebuilding
    m_data.resizeLineVertexCount(m_gridVertexIdx);
  }

  m_gridVertexIdx = m_data.lineVertexCount();

  // setup grid
  for (int x = m_gridConfig.minX; x <= m_gridConfig.maxX; x += m_gridConfig.step) {
    for (int y = m_gridConfig.minY; y <= m_gridConfig.maxY; y += m_gridConfig.step) {
      // parallel to x
      m_data.addLine(QVector3D(-x, y, 0), QVector3D(x, y, 0), m_gridConfig.color);

      // parallel to y
      m_data.addLine(QVector3D(x, -y, 0), QVector3D(x, y, 0), m_gridConfig.color);
    }
  }

  m_axesVertexIdx = m_data.lineVertexCount();

  // setup coordinate axes
  const float &length = m_axesConfig.length;
  const float &arrSize = m_axesConfig.arrowSize;

  // x (red)
  m_data.addLine(QVector3D(-length, 0, 0), QVector3D(length, 0, 0), m_axesConfig.colorX);

  // arrow
  m_data.addLine(QVector3D(length, 0, 0), QVector3D(length - arrSize, arrSize / 2, 0), m_axesConfig.colorX);
  m_data.addLine(QVector3D(length, 0, 0), QVector3D(length - arrSize, -arrSize / 2, 0), m_axesConfig.colorX);

  // y (green)
  m_data.addLine(QVector3D(0, -length, 0), QVector3D(0, length, 0), m_axesConfig.colorY);

  // arrow
  m_data.addLine(QVector3D(0, length, 0), QVector3D(arrSize / 2, length - arrSize, 0), m_axesConfig.colorY);
  m_data.addLine(QVector3D(0, length, 0), QVector3D(-arrSize / 2, length - arrSize, 0), m_axesConfig.colorY);

  // z (blue)
  m_data.addLine(QVector3D(0, 0, -length), QVector3D(0, 0, length), m_axesConfig.colorZ);

  // arrow
  m_data.addLine(QVector3D(0, 0, length), QVector3D(arrSize / 2, 0, length - arrSize), m_axesConfig.colorZ);
  m_data.addLine(QVector3D(0, 0, length), QVector3D(-arrSize / 2, 0, length - arrSize), m_axesConfig.colorZ);
}



void QGLViewer::setupGL() {
  if (m_program == nullptr)
    return;

  initializeGridAndAxes();

  m_trisVao.bind();
  m_program->bind();

  // Setup our vertex buffer objects.

  // scene objects are in data VBO
  m_trisVbo.bind();
  m_trisVbo.allocate(m_data.triangleConstData(), m_data.triangleDataSize() * sizeof(GLfloat));

  // Store the vertex attribute bindings for the program.
  setupVertexAttribs();
  m_trisVbo.release();

  m_trisVao.release();


  m_linesVao.bind();
  m_linesVbo.bind();
  m_linesVbo.allocate(m_data.lineConstData(), m_data.lineDataSize() * sizeof(GLfloat));
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
  m_program->setUniformValue(m_mvpMatrixLoc, m_camera->toMatrix());


  /* It doesn't matter if the vertex attributes are all from one buffer or multiple buffers,
   * and we don't need to bind any particular vertex buffer when drawing; all the glDraw* functions
   * care about is which vertex attribute arrays are enabled.
   */

  m_trisVao.bind();
  // render as wireframe
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawArrays(GL_TRIANGLES, 0, m_data.triangleVertexCount());
  m_trisVao.release();

  glLineWidth(2);
  m_linesVao.bind();
  glDrawArrays(GL_LINES, 0, m_gridVertexIdx);
  m_linesVao.release();

  if (m_drawGrid && m_gridVertexIdx > -1) {
    glLineWidth(0.5f);
    m_linesVao.bind();
    glDrawArrays(GL_LINES, m_gridVertexIdx, m_axesVertexIdx - m_gridVertexIdx);
    m_linesVao.release();
  }

  if (m_drawAxes && m_axesVertexIdx > -1) {
    glLineWidth(3);
    m_linesVao.bind();
    glDrawArrays(GL_LINES, m_axesVertexIdx, m_data.lineVertexCount() - m_axesVertexIdx);
    m_linesVao.release();
  }

  m_program->release();
}

void QGLViewer::resizeGL(int w, int h) {
  m_camera->setAspectRatio(GLfloat(w) / h);
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
      m_camera->reset();
      break;
    case Qt::Key_P:
      m_camera->setProjectionMode(ProjectionMode::Perspective);
      break;
    case Qt::Key_O:
      m_camera->setProjectionMode(ProjectionMode::Orthographic);
      break;
    case Qt::Key_F:
      m_camera->setCameraMode(CameraMode::Free);
      break;
    case Qt::Key_T:
      m_camera->setCameraMode(CameraMode::Target);
      break;
    case Qt::Key_L:  // log current camera data
      qDebug() << *m_camera;
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

  int upDown = m_camera->upsideDown() ? -1 : 1;

  if (event->buttons() & Qt::LeftButton) {
    if (m_camera->cameraMode() == CameraMode::Free) {
      m_camera->rotate(-0.2f * dx, m_camera->upVector());
      m_camera->rotate(-0.2f * dy, m_camera->rightVector());
    } else {
      // if the up vector actually points down, reverse rotation
      m_camera->rotate(-0.2f * dx, upDown * m_camera->worldUpVector());
      m_camera->rotate(-0.2f * dy, upDown * QVector3D::crossProduct(m_camera->forwardVector(), m_camera->worldUpVector()));
    }
  } else if (event->buttons() & Qt::RightButton) {
    if (m_camera->cameraMode() == CameraMode::Free) {
      m_camera->rotate(0.2f * dx, m_camera->forwardVector());
      m_camera->rotate(-0.2f * dy, m_camera->rightVector());
    } else {
      m_camera->rotate(-0.2f * dx, m_camera->forwardVector());
      m_camera->rotate(-0.2f * dy, upDown * QVector3D::crossProduct(m_camera->forwardVector(), m_camera->worldUpVector()));
    }
  } else if (event->buttons() & Qt::MiddleButton) {
    if (m_camera->cameraMode() == CameraMode::Free) {
      dx *= -1;
      dy *= -1;
    }

    m_camera->translate(-dx * m_camera->rightVector());
    m_camera->translate( dy * m_camera->upVector());
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

  m_camera->translate(factor * m_camera->forwardVector());

  event->accept();
  update();
}
