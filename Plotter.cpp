#include "Plotter.h"
#include "ExprLib.h"
#include "Expression.h"
#include "RobotoMono_font.h"
#include "Roboto_font.h"
#include <atomic>
#include <chrono>
#include <imgui-SFML.h>
#include <imgui.h>
#include <memory>
#include <misc/cpp/imgui_stdlib.h>
#include <mutex>
#include <thread>

Plotter::Plotter(sf::Vector2u size, sf::Vector2f center)
  : _window(sf::RenderWindow(sf::VideoMode(size), "Plotter"))
  , _graph(size, center)
  , _size(size)
{
  if (!ImGui::SFML::Init(_window)) {
    std::cerr << "Failed to initialize ImGui-SFML!\n";
  }

  _mousePressed = false;
  _integrateNumeric = false;
  _cursorLogicalPosition = { 0, 0 };
  _calcNeeded = false;
  _pointsAvailable = false;
  _calcCancelled = false;
  _numericResult = 0;
  _lowerBound = 0;
  _upperBound = 0;

  // Set more neater font
  ImFontConfig fontCfg;
  fontCfg.FontDataOwnedByAtlas = false;
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->AddFontFromMemoryTTF(
    Roboto_variable_ttf, Roboto_variable_ttf_len, 16.0f, &fontCfg);
  io.Fonts->AddFontFromMemoryTTF(
    RobotoMono_variable_ttf, RobotoMono_variable_ttf_len, 16.0f, &fontCfg);
  bool res = ImGui::SFML::UpdateFontTexture();
  // Disable ImGui .ini and log files
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;

  // Set example expression
  std::unique_ptr<Expression> expr =
    ExprLib::CreateExpression("sqrt(x)", { "x" });
  ExprLib::SetExpression(std::move(expr));
  _exprStr = ExprLib::GetCurrentExpressionString();

  _window.setFramerateLimit(60);
}

void
Plotter::Run()
{
  std::thread _calcThread(&Plotter::CalculatorThread, this);
  sf::Clock deltaClock;
  while (_window.isOpen()) {
    ProcessEvents(deltaClock);
    // Since imgui is immediate and doesnt retain state, we need to call to
    // update plotter's state based on widgets events before actual calculations
    DrawGUI();
    _calcNeeded.store(true, std::memory_order_release);
    Render();
  }

  _calcThread.join();
}

void
Plotter::OnWindowClose()
{
  _calcCancelled.store(true, std::memory_order_release);
  _window.close();
}

void
Plotter::OnKeyPress(const sf::Event::KeyPressed& event)
{
}

void
Plotter::OnResize(const sf::Vector2u& newSize)
{
  _size = newSize;
  _calcNeeded.store(true);
  std::lock_guard<std::mutex> _lock(_graphMutex);
  _graph.Resize(_size);
}

void
Plotter::OnMousePress(const sf::Mouse::Button& button,
                      const sf::Vector2i& position)
{
  if (button == sf::Mouse::Button::Left) {
    _mousePressed = true;
    _lastMousePosition = position;
  }
}

void
Plotter::OnMouseMove(const sf::Vector2i& position)
{
  _cursorLogicalPosition = _graph.ScreenToLogical(position);
  if (_mousePressed) {
    _graph.Move(
      { _lastMousePosition.x - position.x, position.y - _lastMousePosition.y });
    _lastMousePosition = position;
    _calcNeeded.store(true);
  }
}

void
Plotter::OnMouseRelease(const sf::Mouse::Button& button)
{
  if (button == sf::Mouse::Button::Left)
    _mousePressed = false;
}

void
Plotter::OnMouseScroll(const sf::Vector2i& position, const float& delta)
{
  float scale = _graph.GetScale();
  _graph.SetPivotPoint(position);
  if (delta < 0)
    _graph.SetScale(scale / 1.1f);
  else if (delta > 0)
    _graph.SetScale(scale * 1.1f);
}

void
Plotter::ProcessEvents(sf::Clock& clock)
{
  while (const auto& e = _window.pollEvent()) {
    ImGui::SFML::ProcessEvent(_window, e.value());

    if (e->is<sf::Event::Closed>())
      OnWindowClose();
    else if (auto ev = e->getIf<sf::Event::KeyPressed>())
      OnKeyPress(*ev);

    if (ImGui::GetIO().WantCaptureMouse)
      continue;

    if (auto ev = e->getIf<sf::Event::Resized>())
      OnResize(ev->size);
    else if (auto ev = e->getIf<sf::Event::MouseButtonPressed>())
      OnMousePress(ev->button, ev->position);
    else if (auto ev = e->getIf<sf::Event::MouseMoved>())
      OnMouseMove(ev->position);
    else if (auto ev = e->getIf<sf::Event::MouseButtonReleased>())
      OnMouseRelease(ev->button);
    else if (auto ev = e->getIf<sf::Event::MouseWheelScrolled>()) {
      if (ev->wheel == sf::Mouse::Wheel::Vertical)
        OnMouseScroll(ev->position, ev->delta);
    }
  }

  ImGui::SFML::Update(_window, clock.restart());
}

void
Plotter::Render()
{
  _window.clear();

  sf::Vector2i pos = _window.getPosition();

  while (!_pointsAvailable.load(std::memory_order_acquire))
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Draw graph if points are available
  std::lock_guard<std::mutex> _lock(_graphMutex);
  // Get graph texture
  _graph.Draw(ExprLib::GetPoints(), ExprLib::GetPointsCount());
  // Get graph size
  sf::Vector2u graphSize = _graph.GetSize();
  // Set window view that way so we can draw graph correctly
  _window.setView(sf::View(
    { graphSize.x / 2.0f + _graphOffset.x,
      graphSize.y / 2.0f + _graphOffset.y },
    { static_cast<float>(graphSize.x), static_cast<float>(graphSize.y) }));
  _window.draw(_graph.GetSprite());

  _window.setView(_window.getDefaultView());
  ImGui::SFML::Render(_window);

  _window.display();
}

void
Plotter::DrawGUI()
{
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoScrollbar |
                           ImGuiWindowFlags_AlwaysAutoResize;
  ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Always);
  ImGui::Begin("##guiPanel", nullptr, flags);

  bool openPopup = false;

  if (ImGui::BeginTable("##table", 1)) {
    float spacing = ImGui::GetStyle().ItemSpacing.x;

    ImGui::TableNextColumn();

    if (ImGui::Button("Build graph!##exprButton")) {
      if (_exprStr[0] != '\0') {
        // Try to create new expression out of input string
        std::unique_ptr<Expression> expr =
          ExprLib::CreateExpression(_exprStr, { "x" });
        if (expr != nullptr) {
          // If expressions are syntactically not equal, then add set new
          // expression
          if (!ExprLib::CompareWithCurrentExpr(expr->GetExpressionString()))
            ExprLib::SetExpression(std::move(expr));
        } else {
          _error = ExprLib::GetLastError();
          openPopup = true;
        }
      } else {
        _error = "Empty expression!";
        openPopup = true;
      }
    }

    ImGui::SetNextItemWidth(200.0f);
    ImGui::SameLine(0, spacing);
    ImGui::InputTextWithHint("##exprInput",
                             "Expression...",
                             &_exprStr,
                             ImGuiInputTextFlags_ElideLeft |
                               ImGuiInputTextFlags_CallbackHistory,
                             (ImGuiInputTextCallback)ExprInputHistoryCallback,
                             this);

    ImGui::SameLine(0, spacing);
    if (ImGui::Button("Derivate##derivateButton")) {
      if (_integrationVariable.empty()) {
        _error = "variable not set";
        openPopup = true;
      } else {
        // Try to derivate current expression by X variable and set derived
        // expression as current, this sometimes may fail if derivation is too
        // computationally complex
        std::unique_ptr<Expression> expr =
          ExprLib::CreateDerivative(_integrationVariable);
        if (expr != nullptr) {
          ExprLib::SetExpression(std::move(expr));
          _exprStr = ExprLib::GetCurrentExpressionString();
        } else {
          _error = ExprLib::GetLastError();
          openPopup = true;
        }
      }
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Differentiate currently shown function");

    ImGui::SetNextItemWidth(70.0f);
    ImGui::SameLine(0, spacing);
    ImGui::InputTextWithHint("##integrationVariableInput",
                             "by var...",
                             &_integrationVariable,
                             ImGuiInputTextFlags_ElideLeft);
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Integration/differentiation variable");

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Numeric integration##integrateNumericCheckbox",
                    &_integrateNumeric);

    ImGui::SameLine(0.0f, spacing);
    if (ImGui::Button("Integrate##integrateButton")) {
      if (_integrationVariable.empty()) {
        _error = "variable not set";
        openPopup = true;
      } else {
        if (!_integrateNumeric) {
          // Try to integrate current expression by X with constant C set as 0.
          // This will fail if current expression is not a polynomial
          std::unique_ptr<Expression> expr =
            ExprLib::CreateAntiderivative(_integrationVariable, 0);
          if (expr != nullptr) {
            ExprLib::SetExpression(std::move(expr));
            _exprStr = ExprLib::GetCurrentExpressionString();
          } else {
            _error = ExprLib::GetLastError();
            openPopup = true;
          }
        } else {
          float parsed = 0;
          char* endptr = nullptr;

          // Try to parse x1 (lower bound)
          parsed = std::strtod(_lowerBoundStr.c_str(), &endptr);
          if (*endptr != '\0') {
            _error = "Failed to parse lower bound!";
          } else {
            _lowerBound = parsed;
          }

          endptr = nullptr;
          // Try to parse x2 (upper bound)
          parsed = std::strtod(_upperBoundStr.c_str(), &endptr);
          if (*endptr != '\0') {
            _error = "Failed to parse lower bound!";
          } else {
            _upperBound = parsed;
          }

          std::optional<double> res =
            ExprLib::CalculateIntegral("x", _lowerBound, _upperBound);
          if (res.has_value()) {
            _numericResult = res.value();
          } else {
            _error = ExprLib::GetLastError();
            openPopup = true;
          }
        }
      }
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Integrate currently shown function");

    uint precision = _graph.GetPrecision() > 3 ? _graph.GetPrecision() : 3;

    ImGui::SameLine(0.0f, spacing);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
    ImGui::Text("X: % .*f; Y: % .*f",
                precision,
                _cursorLogicalPosition.x,
                precision,
                _cursorLogicalPosition.y);
    ImGui::PopFont();

    ImGui::SameLine(0.0f, spacing);
    if (ImGui::Button("Reset scale##resetScaleButton"))
      _graph.ResetScale();

    if (_integrateNumeric) {
      float width = ImGui::GetColumnWidth(0);
      float third = (width - 2 * spacing) / 3.0f;

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);

      ImGui::TextUnformatted("x1:");
      ImGui::SetNextItemWidth(third - ImGui::CalcTextSize("x1:").x - spacing);
      ImGui::SameLine(0.0f, spacing);
      ImGui::InputText(
        "##lowerBoundInput", &_lowerBoundStr, ImGuiInputTextFlags_ElideLeft);

      ImGui::SameLine(0.0f, spacing);
      ImGui::TextUnformatted("x2:");
      ImGui::SetNextItemWidth(third - ImGui::CalcTextSize("x2:").x - spacing);
      ImGui::SameLine(0.0f, spacing);
      ImGui::InputText(
        "##upperBoundInput", &_upperBoundStr, ImGuiInputTextFlags_ElideLeft);

      ImGui::SetNextItemWidth(third - ImGui::CalcTextSize("x2:").x - spacing);
      ImGui::SameLine(0.0f, spacing);
      ImGui::Text("I = %f", _numericResult);
    }

    ImGui::EndTable();
  }

  ImGui::End();

  if (openPopup)
    ImGui::OpenPopup("Error!##exprErrorPopup");

  if (ImGui::BeginPopupModal("Error!##exprErrorPopup",
                             nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoMove |
                               ImGuiWindowFlags_NoResize)) {
    ImGui::Text("%s", _error.c_str());
    ImGui::Separator();

    ImVec2 button_size = ImVec2(120, 0);

    float avail = ImGui::GetContentRegionAvail().x;
    float offset = (avail - button_size.x) * 0.5f;
    if (offset > 0.0f)
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

    if (ImGui::Button("Ok##popupOK", button_size)) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  ImGui::PopFont();
}

void
Plotter::CalculatorThread()
{
  while (!_calcCancelled.load(std::memory_order_acquire)) {
    if (_calcNeeded.load(std::memory_order_acquire)) {
      _pointsAvailable.store(false, std::memory_order_release);
      // We use same mutex to lock different logic because here we reading
      // shared bounds, and in graph Resize() we write to this bounds
      std::lock_guard<std::mutex> _lock(_graphMutex);
      sf::Vector2f xBounds = _graph.GetXBounds();
      ExprLib::CalculateExpression(xBounds.x, xBounds.y);
      _pointsAvailable.store(true, std::memory_order_release);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

int
Plotter::ExprInputHistoryCallback(ImGuiInputTextCallbackData* data)
{
  Plotter* plotter = (Plotter*)data->UserData;
  if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
    if (data->EventKey == ImGuiKey_UpArrow) {
      ExprLib::UndoSetExpression();
      plotter->_exprStr = ExprLib::GetCurrentExpressionString();
      data->DeleteChars(0, data->BufTextLen);
      data->InsertChars(0, plotter->_exprStr.c_str());
    } else if (data->EventKey == ImGuiKey_DownArrow) {
      ExprLib::RedoSetExpression();
      plotter->_exprStr = ExprLib::GetCurrentExpressionString();
      data->DeleteChars(0, data->BufTextLen);
      data->InsertChars(0, plotter->_exprStr.c_str());
    }
  }
  return 0;
}