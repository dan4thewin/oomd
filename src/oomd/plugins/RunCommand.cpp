#include "oomd/plugins/RunCommand.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <future>

#include "oomd/Log.h"
#include "oomd/PluginRegistry.h"
#include "oomd/util/SystemMaybe.h"

namespace Oomd {

REGISTER_PLUGIN(run_command, RunCommand::create);

int RunCommand::init(
    const Engine::PluginArgs& pluginArgs,
    const PluginConstructionContext& context) {
  std::string wholeArg;

  argParser_.addArgument("command", command_, true);
  argParser_.addArgument("use_exit_value", useExitValue_);
  argParser_.addArgument("cache_sec", cacheSec_);
  argParser_.addArgument("timeout_msec", timeoutMsec_);
  argParser_.addArgument("argument", wholeArg);

  if (!argParser_.parse(pluginArgs)) {
    return 1;
  }

  args_ = Util::split(wholeArg, '\t');

  c_args_.emplace_back(const_cast<char*>(command_.c_str()));
  for (auto& s : args_)
    c_args_.emplace_back(const_cast<char*>(s.c_str()));
  c_args_.emplace_back(nullptr);

  return 0;
}

// Fork and execute the command with a timeout
SystemMaybe<int> RunCommand::forkexec() {
  pid_t pid = fork();
  if (pid == -1)
    return systemError(errno, "Failed to fork");

  if (pid == 0) {
    // Child process
    execvp(c_args_[0], c_args_.data());
    _Exit(EXIT_FAILURE); // If execvp fails
  }

  // Parent process: Wait for the child process with a timeout
  std::promise<int> result;
  std::future<int> future = result.get_future();

  std::thread waitThread([pid, &result]() {
    int status;
    waitpid(pid, &status, 0);
    result.set_value(status);
  });

  if (future.wait_for(timeoutMsec_) == std::future_status::timeout) {
    // Timeout occurred
    kill(pid, SIGKILL); // Kill the child process
    waitThread.join();
    return systemError(ECANCELED, "Timeout waiting for child process");
  }

  waitThread.join();
  int status = future.get();
  if (status == -1)
    return systemError(status, "Failed to wait for child process");

  return status;
}

Engine::PluginRet RunCommand::run(OomdContext& ctx) {
  using std::chrono::steady_clock;

  const auto now = steady_clock::now();
  const auto diff =
      std::chrono::duration_cast<std::chrono::seconds>(now - start_).count();
  if (diff < cacheSec_) {
    const auto s = ret_ == Engine::PluginRet::CONTINUE ? "CONTINUE" : "STOP";
    OLOG << "using cached result; diff=" << diff << " ret=" << s;
    return ret_; // Skip execution if within cache time
  }

  start_ = now;

  const auto maybe = forkexec();
  if (!maybe) {
    OLOG << maybe.error().what();
    return ret_ = useExitValue_ ? Engine::PluginRet::STOP
                                : Engine::PluginRet::CONTINUE;
  }

  ret_ = useExitValue_ && (!WIFEXITED(*maybe) || WEXITSTATUS(*maybe) != 0)
      ? Engine::PluginRet::STOP // Non-zero exit value or killed
      : Engine::PluginRet::CONTINUE;

  const auto s = ret_ == Engine::PluginRet::CONTINUE ? "CONTINUE" : "STOP";
  OLOG << "use_exit_value=" << useExitValue_ << " status=" << *maybe
       << " exit=" << WEXITSTATUS(*maybe) << " ret=" << s;
  return ret_;
}

} // namespace Oomd
