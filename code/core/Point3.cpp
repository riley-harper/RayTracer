#include "core/Point3.h"

#include <string>

#include "core/Vector3.h"

Vector3 operator-(const Point3& p, const Point3& q) {
  return Vector3(p.X() - q.X(), p.Y() - q.Y(), p.Z() - q.Z());
}

Point3 operator+(const Point3& p, const Vector3& v) {
  return Point3(p.X() + v.X(), p.Y() + v.Y(), p.Z() + v.Z());
}

std::string ToString(const Point3& p) {
  return "(" + std::to_string(p.X()) + ", " + std::to_string(p.Y()) + ", " + std::to_string(p.Z()) + ")";
}
