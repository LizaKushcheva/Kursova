#pragma once
#include "ExpressionCalculator.h"

namespace ExprLib {
std::unique_ptr<Expression>
CreateExpression(const std::string& expr_str,
                 const std::vector<std::string>& variables);

std::string
GetLastError();

void
SetNPoints(uint npoints);

// Get vector containing last calculation results
std::vector<Point>&
GetPoints();

// Get actual count of points vector
size_t
GetPointsCount();

// compare two expressions if they syntactically equal
bool
CompareWithCurrentExpr(std::string exprStr);

// Get a derivative of expression
std::unique_ptr<Expression>
CreateDerivative(const std::string& variable);

// Get antiderivative of current expression. Works only for polynomials
std::unique_ptr<Expression>
CreateAntiderivative(const std::string& variable, double C);

// Calculate integral of expression with given bounds and integration variable
std::optional<double>
CalculateIntegral(const std::string& variable,
                  double lowerBound,
                  double upperBound);

// Get string representation of current expression
std::string
GetCurrentExpressionString();

// Set current expression
void
SetExpression(std::unique_ptr<Expression> expr);

// Undo expression setting
void
UndoSetExpression();

// Redo expression setting
void
RedoSetExpression();

// Calculate current expression with given boundaries
std::vector<Point>&
CalculateExpression(double x1, double x2);
}