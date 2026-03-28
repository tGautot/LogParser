#include <catch2/catch_session.hpp>

int g_log_level = 0;

int main(int argc, char* argv[]) {
  Catch::Session session;

  using namespace Catch::Clara;
  auto cli = session.cli()
      | Opt(g_log_level, "level")
          ["-L"]["--log-level"]
          ("Set the logging level (default: 0, max: 9)");

  session.cli(cli);

  int rc = session.applyCommandLine(argc, argv);
  if (rc != 0)
    return rc;

  return session.run();
}
