#include "../src/Expression.h"
#include "../src/ExpressionCalculator.h"
#include <exception>

ExpressionCalculator _calc = ExpressionCalculator(100);

bool
TestExpr(const std::string& expr_str,
         const std::vector<std::string>& variables,
         const std::vector<std::string>& args,
         const std::string& deriv_name)
{
  auto expr = Expression::CreateExpression(expr_str, variables);

  if (!expr) {
    std::cout << "Error: " << Expression::GetErrorString() << "\n";
    return false;
  }

  std::cout << "Created expression: " << expr->GetExpressionString() << "\n";

  auto evaluated_expr = expr->EvaluateExpression(args);

  if (!evaluated_expr.has_value()) {
    std::cout << "Error: " << Expression::GetErrorString() << "\n";
  } else {
    std::cout << "Calculated expression: " << evaluated_expr.value() << "\n";
  }

  auto deriv_expr = expr->CreateDerivative(deriv_name);

  if (!deriv_expr) {
    std::cout << "Error: " << Expression::GetErrorString() << "\n";
  } else {
    std::cout << "Got derivative by " << deriv_name << ": "
              << deriv_expr->GetExpressionString() << "\n";
  }

  // Try to get antiderivative
  auto antideriv_expr = expr->CreateAntiderivative(deriv_name, 0);

  if (!antideriv_expr) {
    std::cout << "Error: " << Expression::GetErrorString() << "\n";
  } else {
    std::cout << "Got antiderivative by " << deriv_name << ": "
              << antideriv_expr->GetExpressionString() << "\n";
  }

  // Try to integrate expression in bounds [-100;100]
  auto numeric_integral = expr->CalculateIntegral(deriv_name, -100, 100);

  if (!numeric_integral) {
    std::cout << "Error: " << Expression::GetErrorString() << "\n";
  } else {
    std::cout << "Calculated integral: " << numeric_integral.value() << "\n";
  }

  std::cout << "Testing undo():\n";
  _calc.SetExpression(std::move(expr));
  _calc.UndoSetExpression();
  std::cout << "Expression after undo(): " << _calc.GetCurrentExpressionString()
            << "\n";

  std::cout << "Testing redo():\n";
  _calc.RedoSetExpression();
  std::cout << "Expression after redo(): " << _calc.GetCurrentExpressionString()
            << "\n";

  return true;
}

int
main()
{
  try {
    TestExpr("2*5*sqrt(x)+4", { "x" }, { "-1" }, "x");
    TestExpr("x^2+y^2", { "x", "y" }, { "3", "4" }, "x");
    TestExpr("sin(x^2)", { "x" }, { "2" }, "x");
  } catch (const std::exception& ex) {
    std::cout << "Exception: " << ex.what() << "\n";
  }
}