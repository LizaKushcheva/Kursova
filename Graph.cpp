#include "Graph.h"
#include "Roboto_font.h"
#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <cstdio>
#include <mutex>

Graph::Graph(sf::Vector2u size, sf::Vector2f center)
  : _size(size)
  , _sampleUnitLabel(_gridTextFont)
{
  if (!_gridTextFont.openFromMemory(Roboto_variable_ttf,
                                    Roboto_variable_ttf_len)) {
    std::cerr << "Failed to load grid labels font!\n";
    exit(1);
  }

  _backBuffer = sf::RenderTexture(_size);
  _vertices = sf::VertexArray(sf::PrimitiveType::LineStrip);
  _gridVerticesArray = sf::VertexArray(sf::PrimitiveType::Lines);
  _graphColor = sf::Color::Red;
  _gridColor = sf::Color(128, 128, 128, 255);
  _axisColor = sf::Color::Black;

  _unitLabelPattern = "% .*f";

  _pixelsPerUnit = 80;
  _scaledPixelsPerUnit = _pixelsPerUnit;
  _pivotPoint = center;

  // Setup labels vector, width/height of symbol and sample text object
  auto glyph = _gridTextFont.getGlyph('A', _fontSymbolSize, false);
  _fontSymbolWidth = glyph.bounds.size.x;
  _fontSymbolHeight = glyph.bounds.size.y;
  _sampleUnitLabel.setFont(_gridTextFont);
  _sampleUnitLabel.setCharacterSize(_fontSymbolSize);
  _sampleUnitLabel.setFillColor(_axisColor);
  // add 2 to account for labels on edges due to float inaccuracy
  _gridLabels.resize((size.x / _scaledPixelsPerUnit) +
                       (size.y / _scaledPixelsPerUnit) + 2,
                     _sampleUnitLabel);

  _horizontalLabelsOffsetY = _axisLineThickness;
  _verticalLabelsOffsetY = _axisLineThickness;

  // Adjust visible bounds
  _xBounds.x = _pivotPoint.x - (_size.x / (_scaledPixelsPerUnit * 2.0f));
  _xBounds.y = _pivotPoint.x + (_size.x / (_scaledPixelsPerUnit * 2.0f));
  _yBounds.x = _pivotPoint.y - (_size.y / (_scaledPixelsPerUnit * 2.0f));
  _yBounds.y = _pivotPoint.y + (_size.y / (_scaledPixelsPerUnit * 2.0f));
}

void
Graph::SetScale(float scale)
{
  std::lock_guard<std::mutex> lock(_bufferMutex);

  double ratio = _scale / scale;
  int newExp = floor(log2(scale));
  int oldExp = floor(log2(_scale));

  // If new scaling is bigger, then zooming in, increase precision
  if (newExp > oldExp && scale > 1.0f) {
    ++_gridLabelTextPrecision;
  }
  // If previous scale is bigger that newer, then we're zooming out, decrease
  // precision
  else if (newExp < oldExp && _gridLabelTextPrecision > 0) {
    --_gridLabelTextPrecision;
  }

  _scale = scale;
  // Get nice step scale of 1,2,4,8.. instead of arbitrary like 1.1, 1.2 etc
  // by finding floor'ed exponent, and then dividing logical step by 2^exp
  _scaledStep = _baseStep / std::pow(2.0f, newExp);
  _scaledPixelsPerUnit = _scaledPixelsPerUnit / ratio;
  if (_scaledPixelsPerUnit > 2 * _pixelsPerUnit)
    _scaledPixelsPerUnit = _pixelsPerUnit;
  else if (_scaledPixelsPerUnit < _pixelsPerUnit)
    _scaledPixelsPerUnit = 2 * _pixelsPerUnit;

  _xBounds.x =
    _pivotPoint.x - _pivotPointScreen.x * _scaledStep / _scaledPixelsPerUnit;
  _xBounds.y = _pivotPoint.x + (_size.x - _pivotPointScreen.x) * _scaledStep /
                                 _scaledPixelsPerUnit;
  _yBounds.x = _pivotPoint.y - (_size.y - _pivotPointScreen.y) * _scaledStep /
                                 _scaledPixelsPerUnit;
  _yBounds.y =
    _pivotPoint.y + _pivotPointScreen.y * _scaledStep / _scaledPixelsPerUnit;

  // add 2 to account for labels on edges due to float inaccuracy
  _gridLabels.resize((_size.x / _scaledPixelsPerUnit) +
                       (_size.y / _scaledPixelsPerUnit) + 2,
                     _sampleUnitLabel);
}

void
Graph::Resize(sf::Vector2u size)
{
  std::lock_guard<std::mutex> lock(_bufferMutex);
  std::lock_guard<std::mutex> front_lock(_frontBufferMutex);
  bool res = _frontBuffer.resize(size);
  res = _backBuffer.resize(size);
  _size = size;
  // add 2 to account for labels on edges due to float inaccuracy
  _gridLabels.resize((size.x / _scaledPixelsPerUnit) +
                       (size.y / _scaledPixelsPerUnit) + 2,
                     _sampleUnitLabel);
  // Set new right and bottom corners of logical view, so that its anchored to
  // top-left corner
  sf::Vector2f newCorner =
    ScreenToLogical({ static_cast<int>(size.x), static_cast<int>(size.y) });
  _xBounds.y = newCorner.x;
  _yBounds.x = newCorner.y;
}

void
Graph::Draw(std::vector<Point>& points, size_t count)
{
  std::lock_guard<std::mutex> lock(_bufferMutex);
  // Clear out texture area with white color
  _backBuffer.clear(sf::Color::White);
  _xAxisVisible =
    (_yBounds.x + _fontSymbolHeight * _scaledStep / _scaledPixelsPerUnit <=
       0.0f &&
     _yBounds.y >= 0.0f);
  _yAxisVisible =
    (_xBounds.x + _fontSymbolWidth * _scaledStep / _scaledPixelsPerUnit <=
       0.0f &&
     _xBounds.y >= 0.0f);

  DrawGrid();
  DrawAxisLines();
  DrawLabels();

  // Draw function graph
  for (size_t i = 0; i < count; ++i) {
    _vertices.append({ LogicalToScreen({ static_cast<float>(points[i].x),
                                         static_cast<float>(points[i].y) }),
                       _graphColor });
    if (points[i].lineEnd) {
      _backBuffer.draw(_vertices);
      _vertices.clear();
    }
  }

  _backBuffer.display();

  std::lock_guard<std::mutex> front_lock(_frontBufferMutex);
  std::swap(_frontBuffer, _backBuffer);
}

sf::Vector2f
Graph::LogicalToScreen(const sf::Vector2f point)
{
  return { (point.x - _xBounds.x) * _scaledPixelsPerUnit / _scaledStep,
           (_yBounds.y - point.y) * _scaledPixelsPerUnit / _scaledStep };
}

sf::Vector2f
Graph::ScreenToLogical(const sf::Vector2i& point)
{
  return { _xBounds.x + point.x * _scaledStep / _scaledPixelsPerUnit,
           _yBounds.y - point.y * _scaledStep / _scaledPixelsPerUnit };
}

void
Graph::DrawGrid()
{
  // Draw vertical lines
  float linePos = _xBounds.x - std::fmod(_xBounds.x, _scaledStep);
  // Convert to screen coords
  linePos = LogicalToScreen({ linePos, 0 }).x;
  _gridVerticesArray.clear();
  // Set vertical lines
  while (linePos < _size.x) {
    _gridVerticesArray.append({ { linePos, 0 }, _gridColor });
    _gridVerticesArray.append(
      { { linePos, static_cast<float>(_size.y) }, _gridColor });
    linePos += _scaledPixelsPerUnit;
  }
  linePos = _yBounds.y - std::fmod(_yBounds.y, _scaledStep);
  linePos = LogicalToScreen({ 0, linePos }).y;
  // Set horizontal lines
  while (linePos < _size.y) {
    _gridVerticesArray.append({ { 0, linePos }, _gridColor });
    _gridVerticesArray.append(
      { { static_cast<float>(_size.x), linePos }, _gridColor });
    linePos += _scaledPixelsPerUnit;
  }

  _backBuffer.draw(_gridVerticesArray);
}

void
Graph::DrawAxisLines()
{
  sf::Vector2f screen = LogicalToScreen({ 0, 0 });
  sf::RectangleShape line({ _axisLineThickness, static_cast<float>(_size.y) });
  line.setFillColor(_axisColor);
  // Draw horizontal axis
  if (_yAxisVisible) {
    line.setPosition({ screen.x - _axisLineThickness / 2.0f, 0.0f });
    _backBuffer.draw(line);
  }
  // Draw vertical axis
  if (_xAxisVisible) {
    line.setSize({ static_cast<float>(_size.x), _axisLineThickness });
    line.setPosition({ 0.0f, screen.y - _axisLineThickness / 2.0f });
    _backBuffer.draw(line);
  }
}

void
Graph::DrawLabels()
{
  int i = 0;
  float linePos;
  // Find constant Y position for drawing labels - if X axis is visible, then
  // draw on it, otherwise draw on the bottom of screen
  float yPos = _xAxisVisible
                 ? LogicalToScreen({ 0, 0 }).y + _verticalLabelsOffsetY
                 : _size.y - (_verticalLabelsOffsetY + _fontSymbolHeight * 2);
  float leftMostX = _xBounds.x - std::fmod(_xBounds.x, _scaledStep);
  float epsilon = 1e-9;
  // Draw unit labels on horizontal grid lines
  linePos = LogicalToScreen({ leftMostX, 0 }).x;

  while (linePos < _size.x) {
    // do not draw 0 label on X axis
    if (fabs(leftMostX) > epsilon) {
      // Calculate text width and offset label that way so it gets centered
      // around its grid line
      int chars = snprintf(_gridLabelBuf,
                           sizeof(_gridLabelBuf),
                           _unitLabelPattern.c_str(),
                           _gridLabelTextPrecision,
                           leftMostX);

      float offsetX = chars * _fontSymbolWidth / 2.0f;
      // Set position of text and string
      _gridLabels[i].setPosition({ std::round(linePos - offsetX), yPos });
      _gridLabels[i].setString(_gridLabelBuf);
      _backBuffer.draw(_gridLabels[i]);
    }

    // Move to other grid line
    linePos += _scaledPixelsPerUnit;
    leftMostX += _scaledStep;
    ++i;
  }

  // Find constant X position for drawing labels - if Y axis is visible, then
  // draw on it, otherwise draw on the right edge of screen
  float xPos = _yAxisVisible ? LogicalToScreen({ 0, 0 }).x
                             : _size.x - _horizontalLabelsOffsetY;
  float topMostY = _yBounds.y - std::fmod(_yBounds.y, _scaledStep);
  // Draw unit labels on vertical grid lines
  linePos = LogicalToScreen({ static_cast<float>(_size.x), topMostY }).y +
            _verticalLabelsOffsetY;
  while (linePos < _size.y) {
    // do not draw 0 label on Y axis if Y axis is visible
    if (!_yAxisVisible && fabs(topMostY) < epsilon) {
      // Move to other grid line
      linePos += _scaledPixelsPerUnit;
      topMostY -= _scaledStep;
      ++i;
      continue;
    }

    // Calculate text width and offset label that way so it gets centered around
    // its grid line
    int chars = snprintf(_gridLabelBuf,
                         sizeof(_gridLabelBuf),
                         _unitLabelPattern.c_str(),
                         _gridLabelTextPrecision,
                         topMostY);

    float offsetX = chars * _fontSymbolWidth;
    // Set position of text and string
    _gridLabels[i].setPosition({ std::round(xPos - offsetX), linePos });
    _gridLabels[i].setString(_gridLabelBuf);
    _backBuffer.draw(_gridLabels[i]);

    // Move to other grid line
    linePos += _scaledPixelsPerUnit;
    topMostY -= _scaledStep;
    ++i;
  }
}

void
Graph::Move(sf::Vector2i move)
{
  std::lock_guard<std::mutex> lock(_bufferMutex);
  float xMove = move.x * _scaledStep / _scaledPixelsPerUnit;
  float yMove = move.y * _scaledStep / _scaledPixelsPerUnit;
  _xBounds.x += xMove;
  _xBounds.y += xMove;
  _yBounds.x += yMove;
  _yBounds.y += yMove;
}