#include "gldata.h"


void GLData::addVertex(const QVector3D &a, const QVector3D &color, QVector<GLfloat> &data) {
  data.push_back(a.x());
  data.push_back(a.y());
  data.push_back(a.z());

  data.push_back(color.x());
  data.push_back(color.y());
  data.push_back(color.z());
}


void GLData::addLine(const QVector3D &a, const QVector3D &b, const QVector3D &color) {
  addVertex(a, color, m_lines);
  addVertex(b, color, m_lines);
}

void GLData::addTriangle(const QVector3D &a, const QVector3D &b, const QVector3D &c, const QVector3D &color) {
  addVertex(a, color, m_tris);
  addVertex(b, color, m_tris);
  addVertex(c, color, m_tris);
}

void GLData::addCuboid(const QVector3D &u1left, const QVector3D &u1right, const QVector3D &u2left, const QVector3D &u2right,
                    float thickness, float fracGreen, float fracBlue, Sides sides) {
  QVector3D pnormal = QVector3D::normal(u1left, u2left, u1right) * thickness;

  // upper part of cuboid given, normal given => calculate lower part
  QVector3D l1left = u1left + pnormal;
  QVector3D l1right = u1right + pnormal;
  QVector3D l2left = u2left + pnormal;
  QVector3D l2right = u2right + pnormal;

  // top green
  if (sides & TOP) {
    addTriangle(u1left, u1right, u2left, QVector3D(0, 1 - fracGreen, fracBlue));
    addTriangle(u1right, u2right, u2left, QVector3D(0, 1 - fracGreen, fracBlue));
  }

  // right, front blue
  if (sides & RIGHT) {
    addTriangle(u1right, l1right, u2right, QVector3D(0, 0, 1));
    addTriangle(l1right, l2right, u2right, QVector3D(0, 0, 1));
  }

  if (sides & FRONT) {
    addTriangle(u2left, u2right, l2right, QVector3D(0, 0, 1));
    addTriangle(u2left, l2right, l2left, QVector3D(0, 0, 1));
  }

  // left, back yellow
  if (sides & LEFT) {
    addTriangle(u1left, u2left, l1left, QVector3D(1, 1, 0));
    addTriangle(l1left, u2left, l2left, QVector3D(1, 1, 0));
  }

  if (sides & BACK) {
    addTriangle(u1right, u1left, l1left, QVector3D(1, 1, 0));
    addTriangle(u1right, l1left, l1right, QVector3D(1, 1, 0));
  }

  // bottom red
  if (sides & BOTTOM) {
    addTriangle(l1left, l2left, l1right, QVector3D(1 - fracGreen, 0, fracBlue));
    addTriangle(l1right, l2left, l2right, QVector3D(1 - fracGreen, 0, fracBlue));
  }
}
