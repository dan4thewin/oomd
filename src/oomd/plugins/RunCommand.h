#pragma once

#include <chrono>

#include "oomd/engine/BasePlugin.h"

namespace Oomd {

class RunCommand : public Engine::BasePlugin {
 public:
  int init(
      const Engine::PluginArgs& args,
      const PluginConstructionContext& /* unused */) override;

  Engine::PluginRet run(OomdContext& /* unused */) override;

  static RunCommand* create() {
    return new RunCommand();
  }

  ~RunCommand() = default;

 private:
  // Test only
  friend class TestHelper;

  SystemMaybe<int> forkexec();

  std::string command_;
  int64_t cacheSec_{0}; // Default to no caching
  std::chrono::milliseconds timeoutMsec_{std::chrono::milliseconds(500)};
  bool useExitValue_{false};
  std::chrono::time_point<std::chrono::steady_clock> start_{};
  Engine::PluginRet ret_;
  std::vector<std::string> args_;
  std::vector<char*> c_args_;
};

} // namespace Oomd
