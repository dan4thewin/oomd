/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "oomd/plugins/MemAvailable.h"

#include <stdexcept>
#include <string>

#include "oomd/Log.h"
#include "oomd/PluginRegistry.h"
#include "oomd/util/Fs.h"
#include "oomd/util/PluginArgParser.h"
#include "oomd/util/Util.h"

namespace Oomd {

REGISTER_PLUGIN(mem_available, MemAvailable::create);

int MemAvailable::init(
    const Engine::PluginArgs& args,
    const PluginConstructionContext& /* unused */) {
  if (args.find("meminfo_location") != args.end()) {
    meminfo_location_ = args.at("meminfo_location");
  }

  auto meminfoMaybe = Fs::getMeminfo(meminfo_location_);
  if (!meminfoMaybe) {
    OLOG << "Could not read meminfo " << meminfoMaybe.error().what();
    return 1;
  } else if (!meminfoMaybe->count("MemTotal")) {
    OLOG << "meminfo does not contain MemTotal";
    return 1;
  }
  auto memTotal = (*meminfoMaybe)["MemTotal"];

  auto argsCopy = args;
  argsCopy.erase("meminfo_location");

  argParser_.addArgumentCustom(
      "threshold",
      threshold_,
      [memTotal](const std::string& str) {
        int64_t res = 0;
        if (Util::parseSizeOrPercent(str, &res, memTotal) != 0) {
          throw std::invalid_argument("Failed to parse threshold: " + str);
        }
        return res;
      },
      true);

  argParser_.addArgument("duration", duration_, true);

  if (!argParser_.parse(argsCopy)) {
    return 1;
  }

  // Success
  return 0;
}

Engine::PluginRet MemAvailable::run(OomdContext& /* unused */) {
  using std::chrono::steady_clock;

  auto meminfoMaybe = Fs::getMeminfo(meminfo_location_);
  if (!meminfoMaybe) {
    OLOG << "Could not read meminfo";
    return Engine::PluginRet::STOP;
  }

  auto it = meminfoMaybe->find("MemAvailable");
  if (it == meminfoMaybe->end()) {
    OLOG << "meminfo does not contain MemAvailable";
    return Engine::PluginRet::STOP;
  }

  int64_t mem_available = it->second;
  const auto now = steady_clock::now();

  if (mem_available < threshold_) {
    if (hit_thres_at_ == steady_clock::time_point()) {
      hit_thres_at_ = now;
    }

    const auto diff =
        std::chrono::duration_cast<std::chrono::seconds>(now - hit_thres_at_)
            .count();

    if (diff >= duration_) {
      OLOG << "MemAvailable " << mem_available / 1024 / 1024
           << "MB is less than the threshold of " << threshold_ / 1024 / 1024
           << "MB for " << duration_ << " seconds";
      return Engine::PluginRet::CONTINUE;
    }
  } else {
    hit_thres_at_ = steady_clock::time_point();
  }

  return Engine::PluginRet::STOP;
}

} // namespace Oomd
