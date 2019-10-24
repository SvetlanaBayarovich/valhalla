#include "test.h"

#include <iostream>
#include <string>
#include <vector>

#include "baldr/rapidjson_utils.h"
#include "loki/worker.h"
#include "midgard/logging.h"
#include "midgard/util.h"
#include "mjolnir/util.h"
#include "odin/worker.h"
#include "sif/autocost.h"
#include "thor/astar.h"
#include "thor/worker.h"
#include <boost/property_tree/ptree.hpp>

using namespace valhalla;
using namespace valhalla::thor;
using namespace valhalla::odin;
using namespace valhalla::sif;
using namespace valhalla::loki;
using namespace valhalla::baldr;
using namespace valhalla::midgard;
using namespace valhalla::tyr;

#if !defined(VALHALLA_SOURCE_DIR)
#define VALHALLA_SOURCE_DIR
#endif

namespace {

const std::string config_file = "test/test_config_ut";
std::string ways_file = "test_ways_whitelion.bin";
std::string way_nodes_file = "test_way_nodes_whitelion.bin";
std::string access_file = "test_access_whitelion.bin";
std::string from_restriction_file = "test_from_complex_restrictions_whitelion.bin";
std::string to_restriction_file = "test_to_complex_restrictions_whitelion.bin";
std::string bss_file = "test_bss_nodes_whitelion.bin";

boost::property_tree::ptree get_conf() {
  std::stringstream ss;
  ss << R"({
      "mjolnir":{"tile_dir":"test/data/whitelion_tiles", "concurrency": 1},
      "loki":{
        "actions":["route"],
        "logging":{"long_request": 100},
        "service_defaults":{"minimum_reachability": 2,"radius": 10,"search_cutoff": 35000, "node_snap_tolerance": 5, "street_side_tolerance": 5, "heading_tolerance": 60}
      },
      "thor":{"logging":{"long_request": 100}},
      "odin":{"logging":{"long_request": 100}},
      "skadi":{"actons":["height"],"logging":{"long_request": 5}},
      "meili":{"customizable": ["turn_penalty_factor","max_route_distance_factor","max_route_time_factor","search_radius"],
              "mode":"auto","grid":{"cache_size":100240,"size":500},
              "default":{"beta":3,"breakage_distance":2000,"geometry":false,"gps_accuracy":5.0,"interpolation_distance":10,
              "max_route_distance_factor":5,"max_route_time_factor":5,"max_search_radius":200,"route":true,
              "search_radius":15.0,"sigma_z":4.07,"turn_penalty_factor":200}},
      "service_limits": {
        "auto": {"max_distance": 5000000.0, "max_locations": 20,"max_matrix_distance": 400000.0,"max_matrix_locations": 50},
        "auto_shorter": {"max_distance": 5000000.0,"max_locations": 20,"max_matrix_distance": 400000.0,"max_matrix_locations": 50},
        "bicycle": {"max_distance": 500000.0,"max_locations": 50,"max_matrix_distance": 200000.0,"max_matrix_locations": 50},
        "bus": {"max_distance": 5000000.0,"max_locations": 50,"max_matrix_distance": 400000.0,"max_matrix_locations": 50},
        "hov": {"max_distance": 5000000.0,"max_locations": 20,"max_matrix_distance": 400000.0,"max_matrix_locations": 50},
        "isochrone": {"max_contours": 4,"max_distance": 25000.0,"max_locations": 1,"max_time": 120},
        "max_avoid_locations": 50,"max_radius": 200,"max_reachability": 100,"max_alternates":2,
        "multimodal": {"max_distance": 500000.0,"max_locations": 50,"max_matrix_distance": 0.0,"max_matrix_locations": 0},
        "pedestrian": {"max_distance": 250000.0,"max_locations": 50,"max_matrix_distance": 200000.0,"max_matrix_locations": 50,"max_transit_walking_distance": 10000,"min_transit_walking_distance": 1},
        "skadi": {"max_shape": 750000,"min_resample": 10.0},
        "trace": {"max_distance": 200000.0,"max_gps_accuracy": 100.0,"max_search_radius": 100,"max_shape": 16000,"max_best_paths":4,"max_best_paths_shape":100},
        "transit": {"max_distance": 500000.0,"max_locations": 50,"max_matrix_distance": 200000.0,"max_matrix_locations": 50},
        "truck": {"max_distance": 5000000.0,"max_locations": 20,"max_matrix_distance": 400000.0,"max_matrix_locations": 50}
      }
    })";
  boost::property_tree::ptree conf;
  rapidjson::read_json(ss, conf);
  return conf;
}

struct route_tester {
  route_tester()
      : conf(get_conf()), reader(new GraphReader(conf.get_child("mjolnir"))),
        loki_worker(conf, reader), thor_worker(conf, reader), odin_worker(conf) {
  }
  Api test(const std::string& request_json) {
    Api request;
    ParseApi(request_json, valhalla::Options::route, request);
    loki_worker.route(request);
    std::pair<std::list<TripLeg>, std::list<DirectionsLeg>> results;
    thor_worker.route(request);
    odin_worker.narrate(request);
    return request;
  }
  boost::property_tree::ptree conf;
  std::shared_ptr<GraphReader> reader;
  loki_worker_t loki_worker;
  thor_worker_t thor_worker;
  odin_worker_t odin_worker;
};

void build_tiles() {
  auto osmdata =
      valhalla::mjolnir::build_tile_set(get_conf(), {VALHALLA_SOURCE_DIR
                                                     "test/data/whitelion_bristol_uk.osm.pbf"});
}

void test_deadend() {

  // TODO The below call builds the tiles, but the test fails when this runs
  // test passes the _next_ time after tiles were built...
  // Disk syncing issues? Race condition?
  //build_tiles();

  route_tester tester;
  std::string request =
      R"({"locations":[{"lat":51.45562646682483,"lon":-2.5952598452568054},{"lat":51.455143447135974,"lon":-2.5958767533302307}],"costing":"auto"})";

  auto response = tester.test(request);

  //const auto& legs = response.trip().routes(0).legs();
  //const auto& directions = response.directions().routes(0).legs();

  //if (legs.size() != 2 || directions.size() != 2) {
  //  throw std::logic_error("Should have two legs with two sets of directions");
  //}

  //std::vector<std::string> names;
  //for (const auto& d : directions) {
  //  for (const auto& m : d.maneuver()) {
  //    std::string name;
  //    for (const auto& n : m.street_name()) {
  //      name += n.value() + " ";
  //    }
  //    if (!name.empty()) {
  //      name.pop_back();
  //    }
  //    names.push_back(name);
  //    if (m.type() == DirectionsLeg_Maneuver_Type_kUturnRight ||
  //        m.type() == DirectionsLeg_Maneuver_Type_kUturnLeft) {
  //      throw std::logic_error("Should not encounter any u-turns");
  //    }
  //  }
  //}

  //if (names != std::vector<std::string>{"Korianderstraat", "Maanzaadstraat", "", "Maanzaadstraat",
  //                                      "Korianderstraat", ""}) {
  //  throw std::logic_error(
  //      "Should be a destination at the midpoint and reverse the route for the second leg");
  //}
}

void TearDown() {
  // boost::filesystem::remove(to_restriction_file);
}

} // namespace

int main() {
  test::suite suite("bidirectional-a-star-whitelion");

  suite.test(TEST_CASE(test_deadend));

  return suite.tear_down();
}