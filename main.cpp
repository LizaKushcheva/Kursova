#include "Plotter.h"
#include <SFML/Graphics.hpp>

int
main()
{
  Plotter program = Plotter({ 800u, 600u }, { 0, 0 });
  program.Run();

  return 0;
}
