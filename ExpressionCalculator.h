#pragma once
#include "Expression.h"
#include <sys/types.h>
#include <vector>

#define EXPR_HISTORY 10

struct Point
{
  double x;
  double y;
  bool lineEnd;
};

class ExpressionCalculator
{
public:
  // ctor, npoints - number of X points
  ExpressionCalculator(uint npoints);

  // Set number of points to calculate
  inline void SetNPoints(uint npoints)
  {
    _points.resize(npoints);
    _nPoints = npoints;
  }

  // Get vector containing last calculation results
  inline std::vector<Point>& GetPoints() { return _points; }

  // Get actual count of points vector
  inline size_t GetPointsCount() const { return _pointsCount; }

  // compare two expressions if they syntactically equal
  inline bool CompareWithCurrentExpr(std::string exprStr) const
  {
    return exprStr == GetCurrentExpressionString();
  }

  // Get a derivative of expression
  inline std::unique_ptr<Expression> CreateDerivative(
    const std::string& variable)
  {
    return _expressions[_currentExprIndex]->CreateDerivative(variable);
  }

  // Get antiderivative of current expression. Works only for polynomials
  inline std::unique_ptr<Expression> CreateAntiderivative(
    const std::string& variable,
    double C)
  {
    return _expressions[_currentExprIndex]->CreateAntiderivative(variable, C);
  }

  // Calculate integral of expression with given bounds and integration variable
  inline std::optional<double> CalculateIntegral(const std::string& variable,
                                                 double lowerBound,
                                                 double upperBound)
  {
    return _expressions[_currentExprIndex]->CalculateIntegral(
      variable, lowerBound, upperBound);
  }

  // Get string representation of current expression
  inline std::string GetCurrentExpressionString() const
  {
    if (_expressions.empty())
      return "";

    return _expressions[_currentExprIndex]->GetExpressionString();
  }

  // Set current expression
  void SetExpression(std::unique_ptr<Expression> expr);

  // Undo expression setting
  void UndoSetExpression();

  // Redo expression setting
  void RedoSetExpression();

  // Calculate current expression with given boundaries
  std::vector<Point>& CalculateExpression(double x1, double x2);

private:
  ExpressionCalculator() = delete;
  ExpressionCalculator(const ExpressionCalculator&) = delete;
  size_t _currentExprIndex;
  uint _nPoints;
  float _lastMinX;
  float _lastMaxX;
  bool _forceCalc;
  std::vector<Point> _points;
  size_t _pointsCount;
  std::vector<std::unique_ptr<Expression>> _expressions;
};