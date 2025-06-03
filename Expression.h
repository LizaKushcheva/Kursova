#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <ginac.h>

class Expression
{
public:
  // Create expression out of given expression string and variables list. If
  // parsing fails, then return nothing
  static std::unique_ptr<Expression> CreateExpression(
    const std::string& expr_str,
    const std::vector<std::string>& variables);

  inline static std::string GetErrorString() { return _error; }

  inline std::string GetExpressionString() const { return _userString; }

  // Evaluate expression, substituting variables with given values, parsing them
  // before
  std::optional<double> EvaluateExpression(
    const std::vector<std::string>& values);

  // Same but with doubles, skipping parsing
  std::optional<double> EvaluateExpression(const std::vector<double>& values);

  // Get a derivative of expression
  std::unique_ptr<Expression> CreateDerivative(const std::string& variable);

  // Get antiderivative of current expression. Works only for polynomials
  std::unique_ptr<Expression> CreateAntiderivative(const std::string& variable,
                                                   double C);

  // Calculate integral of expression with given bounds and integration variable
  std::optional<double> CalculateIntegral(const std::string& variable,
                                          double lowerBound,
                                          double upperBound);

private:
  Expression() = default;
  GiNaC::ex _expr;
  std::string _userString;
  // Table for quick access to symbol by name
  std::unordered_map<std::string, GiNaC::symbol> _symbols;
  // Symbol list for substitution purposes
  GiNaC::lst _symList;
  static std::string _error;

  // Check if symbol is a valid name
  static bool IsValidSymbolName(const std::string& name);
  // Check if given name is valid and exists in symbols map
  bool CheckSymbolName(const std::string& name);
  // Get all symbolic variables from expression
  void GetSymbolicsFromEx(const GiNaC::ex& expr,
                          std::vector<GiNaC::symbol>& vec);
};
