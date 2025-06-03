#include "ExpressionCalculator.h"
#include <cmath>

ExpressionCalculator::ExpressionCalculator(uint npoints)
  : _nPoints(npoints)
  , _pointsCount(0)
{
  _currentExprIndex = 0;
  _forceCalc = false;
  _points.resize(npoints);
}

void
ExpressionCalculator::SetExpression(std::unique_ptr<Expression> expr)
{
  if (_expressions.size() == EXPR_HISTORY)
    _expressions.erase(_expressions.begin());

  _expressions.push_back(std::move(expr));
  _currentExprIndex = _expressions.size() - 1;
  _forceCalc = true;
}

void
ExpressionCalculator::UndoSetExpression()
{
  // If current index is bigger than 0, then "rewind" to previous expression
  if (_currentExprIndex > 0) {
    --_currentExprIndex;
    _forceCalc = true;
  }
}

void
ExpressionCalculator::RedoSetExpression()
{
  // If current index is lower than size of expressions vector - 1, then
  // "switch" to more recent expression
  if (_currentExprIndex < _expressions.size() - 1) {
    ++_currentExprIndex;
    _forceCalc = true;
  }
}

std::vector<Point>&
ExpressionCalculator::CalculateExpression(double x1, double x2)
{
  if (_expressions.empty()) {
    _pointsCount = 0;
    return _points;
  }

  double epsilon = 1e-9;
  if (x1 > x2) {
    double temp = x2;
    x2 = x1;
    x1 = temp;
  }

  if (!_forceCalc && std::fabs(_lastMinX - x1) < epsilon &&
      (std::fabs(_lastMaxX - x2) < epsilon) && _points.size())
    return _points;

  _lastMinX = x1;
  _lastMaxX = x2;
  Expression* expr = _expressions[_currentExprIndex].get();
  double step = (_lastMaxX - _lastMinX) / (_nPoints - 2);
  double x = _lastMinX - epsilon;
  int lastCorrectIndex = 0;
  _pointsCount = 0;

  // Fill points vector with calculated points
  for (int i = 0; i < _nPoints; ++i) {
    std::optional<double> value =
      expr->EvaluateExpression(std::vector<double>{ x });
    if (value.has_value()) {
      _points[_pointsCount] = { x, value.value(), false };
      ++_pointsCount;
      lastCorrectIndex = i;
    } else if (i - 1 == lastCorrectIndex) {
      _points[lastCorrectIndex].lineEnd = true;
    }
    x += step;
  }

  if (_pointsCount)
    _points[_pointsCount - 1].lineEnd = true;

  return _points;
}