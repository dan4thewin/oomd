#pragma once

#include <chrono>

#include "oomd/engine/BasePlugin.h"

namespace Oomd {

class Sleep : public Engine::BasePlugin {
 public:
  int init(
      const Engine::PluginArgs& args,
      const PluginConstructionContext& context) override;

  Engine::PluginRet run(OomdContext& /* unused */) override;

  static Sleep* create() {
    return new Sleep();
  }

  ~Sleep() = default;

  private:
    int duration_;

    std::chrono::steady_clock::time_point start_{};
};

} // namespace Oomd
