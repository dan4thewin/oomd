#include "oomd/plugins/Interdict.h"

#include "oomd/ExitRegistry.h"
#include "oomd/Log.h"
#include "oomd/PluginRegistry.h"

namespace Oomd {

REGISTER_PLUGIN(interdict, Interdict::create);

int Interdict::init(
    const Engine::PluginArgs& args,
    const PluginConstructionContext& context) {
  argParser_.addArgumentCustom(
      "cgroup",
      cgroups_,
      [context](const std::string& cgroupStr) {
        return PluginArgParser::parseCgroup(context, cgroupStr);
      },
      true);
  int64_t pct;
  argParser_.addArgument("memhigh_pct", pct, true);

  if (!argParser_.parse(args)) {
    return 1;
  }

  if (pct < 1 || pct > 99) {
    OLOG << "Percentage not between 0 and 100: " << pct;
    return 1;
  }

  // convert to 128-based percentage to use bit shifts instead of division
  memhigh_factor_ = (pct * 128) / 100;

  REGISTER_EXIT_HOOK(interdict, [this]() { reset(); });

  return 0;
}

std::string int2str(int64_t i) {
  return i == std::numeric_limits<int64_t>::max() ? "max" : std::to_string(i);
}

void Interdict::interdict_one(
    const CgroupContext& ctx,
    bool activate,
    const uint64_t tick,
    std::ostringstream& oss) {
  oss << "cgroup=" << ctx.cgroup().relativePath();

  const auto id = ctx.id().value();
  auto& s = saved_[id];
  if (s.tick == 0) {
    s.memhigh = ctx.memory_high().value_or(0);
    s.dir = &ctx.fd();
    oss << " saved=" << int2str(s.memhigh);
  }
  s.tick = tick;

  const auto current = ctx.current_usage().value_or(0);
  oss << " current=" << current;
  int64_t limit = activate ? (current >> 7) * memhigh_factor_ : s.memhigh;
  if (limit < 4096)
    limit = 4096;

  if (!(activate ^ s.isActive)) {
    oss << " noop" << " isActive=" << s.isActive;
    return;
  }

  if (const auto ret = Fs::writeMemhighAt(ctx.fd(), limit); !ret) {
    oss << " " << ret.error().what();
    return;
  }

  s.isActive = !s.isActive;

  oss << " memory.high=" << int2str(limit) << " isActive=" << s.isActive;
}

Engine::PluginRet Interdict::run(OomdContext& ctx) {
  const auto activate = ctx.getInvokingRuleset().has_value();
  const auto tick = ctx.getCurrentTick();

  for (const auto& cgroupCtx : ctx.addToCacheAndGet(cgroups_)) {
    std::ostringstream oss;
    interdict_one(cgroupCtx, activate, tick, oss);
    OLOG << oss.str();
  }

  // purge data for removed/replaced cgroups periodically
  if (tick % 128 == 0) {
    for (auto it = saved_.begin(); it != saved_.end();) {
      it = it->second.tick == tick ? std::next(it) : saved_.erase(it);
    }
  }

  return Engine::PluginRet::CONTINUE;
}

void Interdict::reset() {
  for (auto it = saved_.begin(); it != saved_.end(); it = saved_.erase(it)) {
    auto& s = it->second;
    Fs::writeMemhighAt(*s.dir, s.memhigh);
  }
}

} // namespace Oomd
