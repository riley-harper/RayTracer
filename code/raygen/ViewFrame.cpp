#include "raygen/ViewFrame.h"

#include <cmath>
#include <cstdlib>

#include "core/Point3.h"
#include "core/Vector3.h"
#include "geom/Ray.h"
#include "logging/Log.h"
#include "raygen/PixelTraceData.h"

// Some <cmath> implementations define this constant, some don't
#ifndef M_PI
constexpr double M_PI = 3.1415926535897932384626;
#endif

float ViewFrame::EyeToFrameDistance() const {
  // Solving the equation tan(half_angle_fov_) = (height_ / 2) / distance
  return (float)height_ / (2 * tanf(half_angle_fov_ * M_PI / 180.0));
}

Point3 ViewFrame::FrameTopLeft() const {
  float d = EyeToFrameDistance();
  Point3 frame_center = eye_pos_ + -d * forward_;
  Point3 frame_top_left = frame_center + -(width_ / 2.0) * right_ + (height_ / 2.0) * up_;
  return frame_top_left;
}

ViewFrame::ViewFrame() {
  width_ = 1;
  height_ = 1;
  eye_pos_ = Point3(0, 0, 0);
  up_ = Vector3(0, 1, 0);
  forward_ = Vector3(0, 0, -1);
  right_ = Vector3(-1, 0 ,0);
  half_angle_fov_ = 45;
  sample_strat_ = SAMPLE_STRATEGY_BASIC;
}

ViewFrame::ViewFrame(int width, int height, const Point3& eye_pos, const Vector3& up, const Vector3& forward, const Vector3& right, float half_angle_fov) {
  width_ = width;
  height_ = height;
  eye_pos_ = eye_pos;
  up_ = up;
  forward_ = forward;
  right_ = right;
  half_angle_fov_ = half_angle_fov;
  sample_strat_ = SAMPLE_STRATEGY_BASIC;
}

void ViewFrame::SetSampleStrategy(SampleStrategy strat) {
  sample_strat_ = strat;
}

std::vector<PixelTraceData> ViewFrame::SamplePoints() const {
  if (sample_strat_ == SAMPLE_STRATEGY_BASIC) {
    Log::Debug("sampling ray targets points with basic strategy");
    return SamplePointsBasic();
  }
  else if (sample_strat_ == SAMPLE_STRATEGY_JITTER) {
    Log::Debug("sampling ray targets with jittered supersample strategy");
    return SamplePointsJittered();
  }
  else { // unrecognized sample strategy
    Log::Warning("unrecognized frame point sample strategy -- defaulting to basic");
    return SamplePointsBasic();
  }
}

std::vector<PixelTraceData> ViewFrame::SamplePointsBasic() const {
  std::vector<PixelTraceData> trace_data_list;
  Point3 frame_top_left = FrameTopLeft();

  // The weight for each ray is just 1.0
  std::vector<float> weights;
  weights.push_back(1.0f);

  std::vector<Point3> targets;

  for (int y = 0; y < height_; y++) {
    Vector3 to_y = -(y + 0.5) * up_;
    for (int x = 0; x < width_; x++) {
      Point3 pixel_center = frame_top_left + (x + 0.5) * right_ + to_y;
      
      // Each pixel has a single target in its center
      targets.push_back(pixel_center);

      PixelTraceData trace_data(x, y, targets, weights);
      trace_data_list.push_back(trace_data);

      targets.clear();
    }
  }

  return trace_data_list;
}

std::vector<PixelTraceData> ViewFrame::SamplePointsJittered() const {
  std::vector<PixelTraceData> trace_data_list;
  Point3 frame_top_left = FrameTopLeft();

  float ray_weight = 1 / (float)num_jitter_samples_;

  std::vector<float> weights;
  for (int i = 0; i < num_jitter_samples_; i++) {
    weights.push_back(ray_weight);
  }

  std::vector<Point3> targets;

  for (int y = 0; y < height_; y++) {
    Vector3 to_y = -y * up_;
    for (int x = 0; x < width_; x++) {
      Point3 pixel_top_left = frame_top_left + x * right_ + to_y;
      for (int i = 0; i < num_jitter_samples_; i++) {
        float x_jitter = rand() / (float)RAND_MAX;
        float y_jitter = rand() / (float)RAND_MAX;

        Point3 point_jitter = pixel_top_left + x_jitter * right_ + -y_jitter * up_;
        targets.push_back(point_jitter);
      }

      PixelTraceData trace_data(x, y, targets, weights);
      trace_data_list.push_back(trace_data);
      targets.clear();
    }
  }

  return trace_data_list;
}

void ViewFrame::SetNumJitterSamples(int num_jitter_samples) {
  num_jitter_samples_ = num_jitter_samples;
}
