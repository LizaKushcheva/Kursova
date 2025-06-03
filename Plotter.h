#pragma once
#include "Graph.h"
#include <SFML/Graphics.hpp>
#include <atomic>
#include <imgui.h>
#include <mutex>
#include <sys/types.h>

class Plotter
{
public:
  Plotter(sf::Vector2u size, sf::Vector2f center);
  void Run();

private:
  Plotter() = delete;
  Plotter(const Plotter&) = delete;
  sf::RenderWindow _window;
  Graph _graph;
  sf::Vector2u _size;
  sf::Vector2i _lastMousePosition;
  // if left mouse button is currently pressed
  bool _mousePressed;
  // Graph position offset relative to window
  sf::Vector2f _graphOffset;
  // Mouse pointer position in graph
  sf::Vector2f _cursorLogicalPosition;
  // If calculation of points needed
  std::atomic_bool _calcNeeded;
  // If points calculated are available
  std::atomic_bool _pointsAvailable;
  // Stop calculator thread flag
  std::atomic_bool _calcCancelled;
  // Graph resize mutex
  std::mutex _graphMutex;
  // Error string
  std::string _error;
  // Input expression string
  std::string _exprStr;
  // Lower integration bound string
  std::string _lowerBoundStr;
  // Upper integration bound string
  std::string _upperBoundStr;
  // Integration/differentiation variable name
  std::string _integrationVariable;
  // "Numeric integration" checkbox value
  bool _integrateNumeric;
  // Lower bound for numeric integration
  float _lowerBound;
  // Upper bound
  float _upperBound;
  // Numeric integration result
  double _numericResult;

  void ProcessEvents(sf::Clock& clock);
  void Render();
  void DrawGUI();
  void CalculatorThread();

  void OnWindowClose();
  void OnKeyPress(const sf::Event::KeyPressed& event);
  void OnResize(const sf::Vector2u& newSize);
  void OnMousePress(const sf::Mouse::Button& button,
                    const sf::Vector2i& position);
  void OnMouseMove(const sf::Vector2i& position);
  void OnMouseRelease(const sf::Mouse::Button& button);
  void OnMouseScroll(const sf::Vector2i& position, const float& delta);
  static int ExprInputHistoryCallback(ImGuiInputTextCallbackData* data);
};