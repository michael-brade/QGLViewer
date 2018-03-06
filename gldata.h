#ifndef GLDATA_H
#define GLDATA_H

#include <qopengl.h>
#include <QVector>
#include <QVector3D>

class GLData
{
public:
  const GLfloat *constData() const { return m_data.constData(); }

  inline int count() const { return m_data.size(); }
  inline int vertexCount() const { return count() / 6; }

  void addVertex(const QVector3D& a, const QVector3D& color);
  void addTriangle(const QVector3D &a, const QVector3D& b, const QVector3D& c, const QVector3D& color);

  enum Sides : char {
    NONE    = 0,
    ALL     = ~0,
    LEFT    = 1 << 0,
    RIGHT   = 1 << 1,
    FRONT   = 1 << 2,
    BACK    = 1 << 3,
    TOP     = 1 << 4,
    BOTTOM  = 1 << 5
  };

  void cuboid(const QVector3D& u1left, const QVector3D& u1right, const QVector3D& u2left, const QVector3D& u2right,
              float thickness, float fracGreen, float fracBlue, Sides sides = ALL);

private:
  QVector<GLfloat> m_data;
};

#endif // GLDATA_H
