#include "oomd/plugins/Sleep.h"

#include "oomd/Log.h"
#include "oomd/PluginRegistry.h"

namespace Oomd {

REGISTER_PLUGIN(sleep, Sleep::create);

int Sleep::init(
    const Engine::PluginArgs& args,
    const PluginConstructionContext& context) {
  using std::chrono::steady_clock;

  argParser_.addArgument("duration", duration_, true);
  if (!argParser_.parse(args)) {
    return 1;
  }

  start_ = steady_clock::now();

  // Success
  return 0;
}

Engine::PluginRet Sleep::run(OomdContext& ctx) {
  using std::chrono::steady_clock;

  auto ret = Engine::PluginRet::STOP;
  const auto now = steady_clock::now();
  const auto diff =
      std::chrono::duration_cast<std::chrono::seconds>(now - start_).count();

  if (diff >= duration_) {
    start_ = steady_clock::now();
    ret = Engine::PluginRet::CONTINUE;
  }

  const auto s = ret == Engine::PluginRet::CONTINUE ? "CONTINUE" : "STOP";
  OLOG << "duration=" << duration_ << " diff=" << diff << " ret=" << s;

  return ret;
}

} // namespace Oomd
