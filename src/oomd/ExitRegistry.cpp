#include "ExitRegistry.h"

namespace Oomd {

ExitRegistry& getExitRegistry() {
  static ExitRegistry r;
  return r;
}

} // namespace Oomd
