#include <iostream>
#include <cstdlib>
#include <string>
#include <mutex> // for status updates with OpenMP

// Define only once
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#include "core/Color.h"
#include "parsing/Parser.h"
#include "parsing/SceneData.h"
#include "logging/Log.h"
#include "image/Image.h"
#include "raygen/ViewFrame.h"
#include "raygen/PixelTraceData.h"
#include "geom/HitInfo.h"
#include "geom/Sphere.h"
#include "geom/Ray.h"
#include "light/Light.h"
#include "spatial/Scene.h"
#include "imagemap/ImageMapBasicClamp.h"

void PrintHelpMsg() {
  std::cout << "usage: ./ray [options] filename\n"
            << "  filename is the path to a scene file to trace\n"
            << "  Options:\n"
            << "  -h, --help          output this help message and exit\n"
            << "  -s, --silent        don't output any log messages\n"
            << "  -v, --verbose       output lots of log and debug messages\n"
            << "  --update-freq n     update on program progress every n%\n";
}

int main(int argc, char* argv[]) {
  Log::SetLevel(LOG_LEVEL_INFO);

  // Process command-line args, skip argv[0] because it's the program name
  std::string infile = "";
  bool found_input_file = false;
  // How long to wait between progress updates in % done
  float trace_update_spacing = 20.0f;

  for (int i = 1; i < argc; i++) {
    if (strcmp("--silent", argv[i]) == 0 || strcmp("-s", argv[i]) == 0) {
      Log::SetLevel(LOG_LEVEL_NONE);
    }
    else if (strcmp("--help", argv[i]) == 0 || strcmp("-h", argv[i]) == 0) {
      PrintHelpMsg();
      exit(0);
    }
    else if (strcmp("--verbose", argv[i]) == 0 || strcmp("-v", argv[i]) == 0) {
      Log::SetLevel(LOG_LEVEL_ALL);
    }
    else if (strcmp("--update-freq", argv[i]) == 0) {
      if (i + 1 >= argc) {
        PrintHelpMsg();
        exit(1);
      }
      try {
        trace_update_spacing = std::stof(argv[i+1]);
      }
      catch (...) {
        PrintHelpMsg();
        exit(1);
      }
      // Consumed 2 strings
      i++;
    }
    else {
      infile = std::string(argv[i]);
      found_input_file = true;
    }
  }

  if (!found_input_file) {
    PrintHelpMsg();
    exit(1);
  }

  // Seed RNG
  srand(time(0));

  Log::Info("reading from input file " + infile);

  Parser parser;
  SceneData sdata;

  try {
    sdata = parser.ParseFile(infile);
  }
  catch (ParseError pe) {
    // If the line number is > 0, then include a ", line #: " string
    // Otherwise, just write a colon ": "
    std::string lineinfo = (pe.lineno > 0 ? ", line " + std::to_string(pe.lineno) + ": " : ": ");
    Log::Fatal("parsing error" + lineinfo + pe.msg);
    exit(1);
  }

  Log::Info("making image of size " + std::to_string(sdata.film_width) + " by " + std::to_string(sdata.film_height));
  // Color all the pixels with the background color at first
  Image img(sdata.film_width, sdata.film_height);
   for (int y = 0; y < img.Height(); y++) {
    for (int x = 0; x < img.Width(); x++) {
      img.SetPixel(x, y, sdata.background_color);
    }
  }

  Log::Info("creating view frame for ray generation");
  ViewFrame view_frame(sdata.film_width, sdata.film_height, sdata.camera_pos, sdata.camera_up, sdata.camera_fwd, sdata.camera_right, sdata.camera_fov_ha);

  view_frame.SetSampleStrategy(sdata.sample_strat);
  view_frame.SetNumJitterSamples(sdata.num_samples_jitter);
  
  std::vector<PixelTraceData> trace_data_list = view_frame.SamplePoints();

  int num_rays = 0;

  for (size_t i = 0; i < trace_data_list.size(); i++) {
    num_rays += trace_data_list.at(i).Targets().size();
  }

  Log::Info("generated " + std::to_string(num_rays) + " rays to trace");
  Log::Info("scene contains " + std::to_string(sdata.primitives.size()) + " primitives");

  std::mutex status_update_mutex;
  int rays_traced = 0;
  float last_update = 0.0f;

  // If there are no primitives to trace, then the program can return
  // an image with just the background color
  // This avoids creating a Scene with 0 primitives, which causes an infinite
  // recursion due to BVH construction
  if (sdata.primitives.size() == 0) {
    Log::Info("no primitives to trace, so all done");
    Log::Info("writing result image to file " + sdata.output_image);
    img.Write(sdata.output_image);
    return 0;
  }

  Log::Info("constructing the scene");
  Scene scene = Scene(sdata.background_color, sdata.primitives);
  Log::Info("scene constructed -- tracing!");

  int num_pixels = trace_data_list.size();
  #pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < num_pixels; i++) {
    PixelTraceData trace_data = trace_data_list.at(i);
    Color pixel_color = Color(0, 0, 0);

    // Trace each ray associated to this pixel
    int num_targets = trace_data.Targets().size();
    for (int i = 0; i < num_targets; i++) {
      Point3 target = trace_data.Targets().at(i);
      float weight = trace_data.Weights().at(i);
      Ray ray = Ray(sdata.camera_pos, target - sdata.camera_pos);

      HitInfo hit_info = scene.FindClosestIntersection(ray);

      if (hit_info.DidIntersect()) {
        Color c = Color(0, 0, 0);
        for (Light* light : sdata.lights) {
          c = c + light->ComputeLighting(ray, hit_info, scene, sdata.max_recurse_depth);
        }
        pixel_color = pixel_color + c * weight;
      }
      else { // no intersection -- background color returned
        pixel_color = pixel_color + scene.BackgroundColor() * weight;
      }

      status_update_mutex.lock();
      rays_traced++;

      float percent_done = (rays_traced / (float)num_rays) * 100;
      if (percent_done >= last_update + trace_update_spacing) {
        Log::Info("tracing update - " + std::to_string(rays_traced) + "/" + std::to_string(num_rays) + " rays traced (~" + std::to_string(int(percent_done)) + "%)");
        last_update = percent_done;
      }
      status_update_mutex.unlock();
    }

    img.SetPixel(trace_data.X(), trace_data.Y(), pixel_color);
  }

  
  Log::Info("applying any post-processing image maps");
  
  // If there are no specified image maps, then just clamp the image to prevent overflow
  if (sdata.image_maps.empty()) {
    ImageMapBasicClamp().ApplyMap(img);
  }
  else {
    for (ImageMap* image_map : sdata.image_maps) {
      image_map->ApplyMap(img);
    }
  }

  Log::Info("writing result image to file " + sdata.output_image);
  img.Write(sdata.output_image);

  return 0;
}
