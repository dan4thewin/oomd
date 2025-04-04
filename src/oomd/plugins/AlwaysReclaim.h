#pragma once

#include "oomd/engine/BasePlugin.h"

namespace Oomd {

class AlwaysReclaim : public Engine::BasePlugin {
 public:
  int init(
      const Engine::PluginArgs& args,
      const PluginConstructionContext& /* unused */) override;

  Engine::PluginRet run(OomdContext& ctx) override;

  static AlwaysReclaim* create() {
    return new AlwaysReclaim();
  }

  ~AlwaysReclaim() = default;

 private:
  void reclaim_one(const Oomd::CgroupContext&);

  std::unordered_set<CgroupPath> cgroups_;
  int64_t reclaim_bytes_{0};
};

} // namespace Oomd
