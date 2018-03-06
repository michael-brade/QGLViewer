#include "camera.h"

#include <cmath>


// directions in local OpenGL coordinates
const QVector3D Camera::LocalForward(0, 0, -1);
const QVector3D Camera::LocalRight  (1, 0,  0);
const QVector3D Camera::LocalUp     (0, 1,  0);



const float PI_q = std::atan(1.0f);  // PI/4

const auto degToRad = [](const float deg) {
  return deg * PI_q / 45;
};


Camera::Camera(const CameraConfig& config)
    : m_config(config),
      aspectRatio(1),
      distance(0),
      m_dirty(true)
{
  QQuaternion localRotation = QQuaternion::fromAxes(-LocalForward, LocalRight, LocalUp);
  QQuaternion worldRotation = QQuaternion::fromAxes(-m_config.WorldForward, m_config.WorldRight, m_config.WorldUp);

  // from world to local OpenGL coordinates
  m_worldToLocal = localRotation * worldRotation.conjugated();
}


// Transform By
void Camera::translate(const QVector3D &dt) {
  m_dirty = true;
  m_translation += dt;

  if (m_config.c_mode == CameraMode::Target) {
    distance = (m_target - m_translation).length();
    updateFrustum();
  }
}

void Camera::rotate(const QQuaternion &dr) {
  m_dirty = true;
  m_rotation = dr * m_rotation;

  if (m_config.c_mode == CameraMode::Target) {
    // rotation around target changes the translation of the camera,
    // but keeps the distance from the target
    QVector3D delta_old = m_target - m_translation;
    QVector3D delta_new = dr.rotatedVector(delta_old);
    m_translation += delta_old - delta_new;
  }
}

// Setters
void Camera::setTranslation(const QVector3D &t) {
  m_dirty = true;
  m_translation = t;

  if (m_config.c_mode == CameraMode::Target) {
    distance = (m_target - m_translation).length();
    updateFrustum();
  }
}

void Camera::setRotation(const QQuaternion &r) {
  m_dirty = true;
  m_rotation = r;

  if (m_config.c_mode == CameraMode::Target) {
    QVector3D delta_old = (m_target - m_translation);
    QVector3D delta_new = forwardVector();
    m_translation += delta_old - delta_new;
  }
}

void Camera::setTarget(const QVector3D &t) {
  m_dirty = true;
  m_target = t;
  QVector3D delta = m_translation - m_target;
  m_rotation = QQuaternion::fromDirection(delta, upVector());
  distance = delta.length();
  updateFrustum();

  emit targetChanged(m_target);
}

void Camera::setCameraMode(CameraMode mode) {
  m_config.c_mode = mode;
  emit cameraModeChanged(mode);

  if (mode == CameraMode::Target) {
    // (re)setting to target chooses a new target in front of the camera
    // with the initial distance + log of current distance from center
    setTarget(m_translation
              + forwardVector() * m_config.initialTranslation.length()
              + forwardVector() * std::log(m_translation.length()));
  }
}

void Camera::setProjectionMode(ProjectionMode mode) {
  m_config.p_mode = mode;
  setAspectRatio(aspectRatio);
  emit projectionModeChanged(mode);
}

void Camera::setAspectRatio(float r) {
  m_dirty = true;
  aspectRatio = r;
  updateFrustum();
}


void Camera::updateFrustum() {
  float zNear = m_config.near;
  float zFar = std::max(2.0f * distance, m_config.far);

  m_projection.setToIdentity();

  if (m_config.p_mode == ProjectionMode::Perspective) {
    m_projection.perspective(m_config.fov, aspectRatio, zNear, zFar);
  } else if (m_config.p_mode == ProjectionMode::Orthographic) {
    float left = -aspectRatio * distance * std::tan(degToRad(m_config.fov / 2.0f));
    float right = aspectRatio * distance * std::tan(degToRad(m_config.fov / 2.0f));
    float bottom = -distance * std::tan(degToRad(m_config.fov / 2.0f));
    float top = distance * std::tan(degToRad(m_config.fov / 2.0f));

    m_projection.ortho(left, right, bottom, top, zNear, zFar);
  }
}

void Camera::reset() {
  setTranslation(m_config.initialTranslation);
  setTarget(0, 0, 0);

  m_rotation = m_worldToLocal.conjugated();
}


// Accessors
const QMatrix4x4 &Camera::toMatrix()
{
  if (m_dirty) {
    m_dirty = false;
    m_world.setToIdentity();
    m_world.rotate(m_rotation.conjugated());
    m_world.translate(-m_translation);
    m_world = m_projection * m_world;
  }
  return m_world;
}

// Queries
QVector3D Camera::forwardVector() const {
  return m_rotation.rotatedVector(LocalForward);
}

QVector3D Camera::rightVector() const {
  return m_rotation.rotatedVector(LocalRight);
}

QVector3D Camera::upVector() const {
  return m_rotation.rotatedVector(LocalUp);
}

bool Camera::upsideDown() const {
  // if the up vector actually points down, we are upside down
  return QVector3D::dotProduct(m_rotation.conjugated() * m_config.WorldUp, m_worldToLocal * m_config.WorldUp) < 0;
}



// Qt Streams
#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const Camera &transform)
{
  dbg << "Camera\n{\n";
  dbg << "Position: <" << transform.translation().x() << ", " << transform.translation().y() << ", " << transform.translation().z() << ">\n";
  dbg << "Rotation: <" << transform.rotation().x() << ", " << transform.rotation().y() << ", " << transform.rotation().z() << " | " << transform.rotation().scalar() << ">\n}";
  return dbg;
}
#endif
