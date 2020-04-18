#ifndef GLDATA_H
#define GLDATA_H

#include <qopengl.h>
#include <QVector>
#include <QVector3D>


/**
 * This class stores points for drawing lines and triangles.
 */
class GLData
{
public:
  const GLfloat *lineConstData() const      { return m_lines.constData(); }
  const GLfloat *triangleConstData() const  { return m_tris.constData(); }

  inline int lineDataSize() const           { return m_lines.size(); }
  inline int triangleDataSize() const       { return m_tris.size(); }

  // one vertex has 6 entries: position (x,y,z) and color (r,g,b)
  inline int lineVertexCount() const        { return lineDataSize() / 6; }
  inline int triangleVertexCount() const    { return triangleDataSize() / 6; }

  inline void resizeLineVertexCount(int size) {
    m_lines.resize(size * 6);
  }


  /**
   * Add a line.
   */
  void addLine(const QVector3D &a, const QVector3D &b, const QVector3D &color);

  /**
   * Add a triangle.
   *
   * Front is usually where the vertices appear in counterclockwise order on the screen.
   */
  void addTriangle(const QVector3D &a, const QVector3D &b, const QVector3D &c, const QVector3D &color);

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

  /**
   * Add a cuboid from top rectangle and thickness.
   *
   * @param thickness
   * @param fracGreen
   * @param fracBlue the fraction of blue that is added to top and bottom green and red
   *
   * @param sides which sides of the cuboid to draw
   */
  void addCuboid(const QVector3D &u1left, const QVector3D &u1right, const QVector3D &u2left, const QVector3D &u2right,
              float thickness, float fracGreen, float fracBlue, Sides sides = ALL);

private:
  // add a vertex a with color to the given data vector
  void addVertex(const QVector3D &a, const QVector3D &color, QVector<GLfloat> &data);

  QVector<GLfloat> m_lines;
  QVector<GLfloat> m_tris;
};

#endif  // GLDATA_H
