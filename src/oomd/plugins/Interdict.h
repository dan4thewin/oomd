#pragma once

#include "oomd/engine/BasePlugin.h"

namespace Oomd {

class Interdict : public Engine::BasePlugin {
 public:
  int init(
      const Engine::PluginArgs& args,
      const PluginConstructionContext& /* unused */) override;

  Engine::PluginRet run(OomdContext& ctx) override;

  static Interdict* create() {
    return new Interdict();
  }

  ~Interdict() = default;

 private:
  // Test only
  friend class TestHelper;

  struct SavedMemhigh {
    SavedMemhigh() : memhigh{0}, isActive{false}, tick{0}, dir{nullptr} {}

    int64_t memhigh;
    bool isActive;
    uint64_t tick;
    const Fs::DirFd* dir;
  };
  std::unordered_map<uint64_t, SavedMemhigh> saved_;

  void interdict_one(
      const Oomd::CgroupContext&,
      bool activate,
      uint64_t tick,
      std::ostringstream& oss);

  void reset();

  std::unordered_set<CgroupPath> cgroups_;
  int64_t memhigh_factor_{0};
};

} // namespace Oomd
