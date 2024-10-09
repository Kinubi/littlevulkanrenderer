#include "application.h"
#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
  lvr::Application app;

  try {
    app.OnStart();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
}
