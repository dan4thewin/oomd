#include "oomd/plugins/AlwaysReclaim.h"

#include "oomd/Log.h"
#include "oomd/PluginRegistry.h"

namespace Oomd {

REGISTER_PLUGIN(always_reclaim, AlwaysReclaim::create);

int AlwaysReclaim::init(
    const Engine::PluginArgs& args,
    const PluginConstructionContext& context) {
  argParser_.addArgumentCustom(
      "cgroup",
      cgroups_,
      [context](const std::string& cgroupStr) {
        return PluginArgParser::parseCgroup(context, cgroupStr);
      },
      true);
  argParser_.addArgument("reclaim_bytes", reclaim_bytes_);

  if (!argParser_.parse(args)) {
    return 1;
  }

  return 0;
}

void AlwaysReclaim::reclaim_one(
    const CgroupContext& target) {
  auto ok = Fs::writeMemReclaimAt(target.fd(), reclaim_bytes_);

  std::ostringstream oss;
  oss << "cgroup=" << target.cgroup().relativePath()
      << " reclaim_bytes=" << reclaim_bytes_ << " ok=" << (bool) ok;
  OLOG << oss.str();
}

Engine::PluginRet AlwaysReclaim::run(OomdContext& ctx) {
  for (const auto& cgroupCtx : ctx.addToCacheAndGet(cgroups_))
    reclaim_one(cgroupCtx);

  return Engine::PluginRet::CONTINUE;
}

} // namespace Oomd
