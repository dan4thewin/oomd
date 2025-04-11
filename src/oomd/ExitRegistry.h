#include <functional>
#include <string>
#include <unordered_map>

namespace Oomd {

class ExitRegistry {
 public:
  using ExitFunction = std::function<void()>;
  using ExitMap = std::unordered_map<std::string, ExitFunction>;

  bool add(const std::string& name, ExitFunction f) {
    if (map_.find(name) != map_.end()) {
      return false;
    }

    map_[name] = f;
    return true;
  }

  void callHooks() {
    for (auto const& data : map_)
      data.second();
  }

 private:
  ExitMap map_;
};

ExitRegistry& getExitRegistry();

#define REGISTER_EXIT_HOOK(hook_name, exit_func) \
  getExitRegistry().add(#hook_name, (exit_func))

} // namespace Oomd
