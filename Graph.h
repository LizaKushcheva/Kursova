#pragma once
#include "ExpressionCalculator.h"
#include <SFML/Graphics.hpp>
#include <mutex>

class Graph
{
public:
  // Graph ctor. size of graph should be set as current screen size to avoid
  // weird behavior, cause this is like "world" size
  Graph(sf::Vector2u size, sf::Vector2f center);

  inline sf::Sprite GetSprite() const
  {
    std::lock_guard<std::mutex> lock(_frontBufferMutex);
    return sf::Sprite(_frontBuffer.getTexture());
  }

  inline sf::Vector2f GetXBounds() const
  {
    std::lock_guard<std::mutex> lock(_bufferMutex);
    return _xBounds;
  }

  inline void ResetScale()
  {
    SetScale(_baseScale);
    _gridLabelTextPrecision = 0;
  }

  inline float GetScale() { return _scale; }

  inline sf::Vector2u GetSize() { return _size; }

  inline void SetPivotPoint(const sf::Vector2i& screenPoint)
  {
    _pivotPointScreen = screenPoint;
    _pivotPoint = ScreenToLogical(screenPoint);
  }

  inline uint GetPrecision() { return _gridLabelTextPrecision; }

  void SetScale(float scale);

  void Resize(sf::Vector2u size);

  void Draw(std::vector<Point>& points, size_t count);

  // "Move" view by given vector in pixels
  void Move(sf::Vector2i move);

  // Convert logical point to point on screen
  sf::Vector2f LogicalToScreen(const sf::Vector2f point);

  // Convert point on screen to logical point
  sf::Vector2f ScreenToLogical(const sf::Vector2i& point);

private:
  Graph() = delete;
  Graph(const Graph&) = delete;
  float _scale = 1.0f;
  float _baseScale = 1.0f;
  uint _pixelsPerUnit;
  uint _scaledPixelsPerUnit;
  float _baseStep = 1.0f;
  float _scaledStep = 1.0f;
  float _axisLineThickness = 4.0f;
  // Width of font symbol glyph
  float _fontSymbolWidth;
  // Height of font symbol glyph
  float _fontSymbolHeight;
  // Size of text symbol
  uint _fontSymbolSize = 16;
  // Is X axis visible on screen
  bool _xAxisVisible;
  // Is Y axis visible on screen
  bool _yAxisVisible;
  // Buffer for formatted grid unit value
  char _gridLabelBuf[32];
  // Width of grid unit text, assuming negative number
  uint _gridLabelTextPrecision = 0;
  // Adjustment offset for labels on horizontal lines
  uint _horizontalLabelsOffsetY;
  // Adjustment offset for labels on vertical lines
  uint _verticalLabelsOffsetY;
  // Pattern string for unit label
  std::string _unitLabelPattern;
  // Size of graph area
  sf::Vector2u _size;
  // Vector of minimal and maximal visible X of graph
  sf::Vector2f _xBounds;
  // Vector of minimal and maximal visible Y of graph
  sf::Vector2f _yBounds;
  // Current pivot point (scaling is done based on it)
  sf::Vector2f _pivotPoint;
  sf::Vector2i _pivotPointScreen;
  sf::RenderTexture _frontBuffer;
  sf::RenderTexture _backBuffer;
  // Grid labels font
  sf::Font _gridTextFont;
  mutable std::mutex _bufferMutex;
  mutable std::mutex _frontBufferMutex;
  sf::Color _graphColor;
  sf::Color _gridColor;
  sf::Color _axisColor;
  // Array of function graph vertices
  sf::VertexArray _vertices;
  // Array of grid lines vertices
  sf::VertexArray _gridVerticesArray;
  // Sample label, which is used for resizing vector of grid labels
  sf::Text _sampleUnitLabel;
  // Grid unit labels vector
  std::vector<sf::Text> _gridLabels;

  void DrawGrid();
  void DrawAxisLines();
  void DrawLabels();
};