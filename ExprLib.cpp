#include "ExprLib.h"
#include "Expression.h"
#include "ExpressionCalculator.h"

ExpressionCalculator _calc = ExpressionCalculator(1000);

std::unique_ptr<Expression>
ExprLib::CreateExpression(const std::string& expr_str,
                          const std::vector<std::string>& variables)
{
  return Expression::CreateExpression(expr_str, variables);
}

std::string
ExprLib::GetLastError()
{
  return Expression::GetErrorString();
}

void
ExprLib::SetNPoints(uint npoints)
{
  _calc.SetNPoints(npoints);
}

// Get vector containing last calculation results
std::vector<Point>&
ExprLib::GetPoints()
{
  return _calc.GetPoints();
}

// Get actual count of points vector
size_t
ExprLib::GetPointsCount()
{
  return _calc.GetPointsCount();
}

// compare two expressions if they syntactically equal
bool
ExprLib::CompareWithCurrentExpr(std::string exprStr)
{
  return _calc.CompareWithCurrentExpr(exprStr);
}

// Get a derivative of expression
std::unique_ptr<Expression>
ExprLib::CreateDerivative(const std::string& variable)
{
  return _calc.CreateDerivative(variable);
}

// Get antiderivative of current expression. Works only for polynomials
std::unique_ptr<Expression>
ExprLib::CreateAntiderivative(const std::string& variable, double C)
{
  return _calc.CreateAntiderivative(variable, C);
}

// Calculate integral of expression with given bounds and integration variable
std::optional<double>
ExprLib::CalculateIntegral(const std::string& variable,
                           double lowerBound,
                           double upperBound)
{
  return _calc.CalculateIntegral(variable, lowerBound, upperBound);
}

// Get string representation of current expression
std::string
ExprLib::GetCurrentExpressionString()
{
  return _calc.GetCurrentExpressionString();
}

// Set current expression
void
ExprLib::SetExpression(std::unique_ptr<Expression> expr)
{
  _calc.SetExpression(std::move(expr));
}

// Undo expression setting
void
ExprLib::UndoSetExpression()
{
  _calc.UndoSetExpression();
}

// Redo expression setting
void
ExprLib::RedoSetExpression()
{
  _calc.RedoSetExpression();
}

// Calculate current expression with given boundaries
std::vector<Point>&
ExprLib::CalculateExpression(double x1, double x2)
{
  return _calc.CalculateExpression(x1, x2);
}