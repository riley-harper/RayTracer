#include "spatial/BVHLeafNode.h"

#include "geom/HitInfo.h"
#include "geom/Ray.h"

HitInfo BVHLeafNode::FindClosestIntersection(const Ray& ray) const {
  return primitive_->FindIntersection(ray);
}
