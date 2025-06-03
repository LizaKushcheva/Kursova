#include "Expression.h"
#include <idx.h>
#include <integral.h>
#include <numeric.h>
#include <optional>
#include <symbol.h>

std::string Expression::_error;

// Check if symbol is a valid name
bool
Expression::IsValidSymbolName(const std::string& name)
{
  if (name.empty())
    return false;

  if (!std::isalpha(name[0]) && name[0] != '_')
    return false;

  for (char c : name) {
    if (!std::isalnum(c) && c != '_')
      return false;
  }

  return true;
}

bool
Expression::CheckSymbolName(const std::string& name)
{
  if (!IsValidSymbolName(name)) {
    _error = "invalid symbol name";
    return false;
  }

  // Try to find symbol by string
  auto sym = _symbols.find(name);
  // If symbol can't be found, then it wasn't defined, exit then
  if (sym == _symbols.end()) {
    _error = "undefined symbols detected";
    return false;
  }

  return true;
}

void
Expression::GetSymbolicsFromEx(const GiNaC::ex& expr,
                               std::vector<GiNaC::symbol>& vec)
{
  for (auto ex : expr) {
    if (GiNaC::is_a<GiNaC::symbol>(ex)) {
      GiNaC::symbol sym = GiNaC::ex_to<GiNaC::symbol>(ex);
      vec.push_back(sym);
    } else {
      GetSymbolicsFromEx(ex, vec);
    }
  }
}

std::unique_ptr<Expression>
Expression::CreateExpression(const std::string& expr_str,
                             const std::vector<std::string>& variables)
{
  Expression wrapper = Expression();

  // Parse variables into symbolic list
  for (auto& var : variables) {
    if (!IsValidSymbolName(var)) {
      _error = "invalid symbol name";
      return nullptr;
    }
    GiNaC::symbol sym = GiNaC::symbol(var);
    wrapper._symbols.emplace(var, sym);
    wrapper._symList.append(sym);
  }

  try {
    wrapper._expr = GiNaC::ex(expr_str, wrapper._symList);
  } catch (const std::exception& ex) {
    _error = ex.what();
    return nullptr;
  }

  wrapper._userString = expr_str;
  return std::make_unique<Expression>(wrapper);
}

std::optional<double>
Expression::EvaluateExpression(const std::vector<std::string>& values)
{
  GiNaC::exmap map;

  // Fill exmap with tuples <symbol, expr> so that symbols can be replaced
  // with expressions
  size_t i = 0;
  for (auto begin = _symbols.begin(); begin != _symbols.end(); ++begin, ++i) {
    GiNaC::ex expr = GiNaC::ex(values[i], _symList);
    map.emplace(begin->second, expr);
  }

  GiNaC::numeric numeric_res;
  try {
    // Substitute expression with given values, assuming that they use same
    // variables as expression does, and then evaluate to float expression
    GiNaC::ex res = _expr.subs(map);

    // Convert result to numeric type
    numeric_res = GiNaC::ex_to<GiNaC::numeric>(res.evalf());

    // If result is not real number, return nullopt
    if (!numeric_res.is_real()) {
      _error = "result of expression is not a real number";
      return std::nullopt;
    }
  } catch (const std::exception& ex) {
    _error = ex.what();
    return std::nullopt;
  }

  return std::make_optional(numeric_res.to_double());
}

std::optional<double>
Expression::EvaluateExpression(const std::vector<double>& values)
{
  GiNaC::exmap map;

  // Fill exmap with tuples <symbol, expr> so that symbols can be replaced
  // with expressions
  size_t i = 0;
  for (auto begin = _symbols.begin(); begin != _symbols.end(); ++begin, ++i) {
    map.emplace(begin->second, values[i]);
  }

  GiNaC::numeric numeric_res;
  try {
    // Substitute expression with given values, assuming that they use same
    // variables as expression does, and then evaluate to float expression
    GiNaC::ex res = _expr.subs(map).evalf();

    if (!GiNaC::is_a<GiNaC::numeric>(res)) {
      _error = "result is not a number";
      return std::nullopt;
    }

    // Convert result to numeric type
    numeric_res = GiNaC::ex_to<GiNaC::numeric>(res);

    // If result is not real number, return nullopt
    if (!numeric_res.is_real()) {
      _error = "result of expression is not a real number";
      return std::nullopt;
    }
  } catch (const std::exception& ex) {
    _error = ex.what();
    return std::nullopt;
  }

  return std::make_optional(numeric_res.to_double());
}

std::unique_ptr<Expression>
Expression::CreateDerivative(const std::string& variable)
{
  if (!CheckSymbolName(variable))
    return nullptr;

  GiNaC::symbol sym = _symbols[variable];

  GiNaC::ex diff = _expr.diff(sym);
  Expression diff_expr = Expression();
  diff_expr._expr = diff;

  // Recreate symbols list
  std::vector<GiNaC::symbol> symbols;
  GetSymbolicsFromEx(diff, symbols);
  for (size_t i = 0; i < symbols.size(); ++i) {
    GiNaC::symbol symbol = symbols[i];
    diff_expr._symbols[symbol.get_name()] = symbol;
    diff_expr._symList.append(symbol);
  }

  std::ostringstream oss;
  oss << diff_expr._expr;

  diff_expr._userString = oss.str();

  return std::make_unique<Expression>(diff_expr);
}

std::unique_ptr<Expression>
Expression::CreateAntiderivative(const std::string& variable, double C)
{
  if (!_expr.is_polynomial(_symList)) {
    _error = "can't integrate non-polinomials, sorry";
    return nullptr;
  }

  if (!CheckSymbolName(variable))
    return nullptr;

  GiNaC::symbol sym = _symbols[variable];
  GiNaC::integral antideriv = GiNaC::integral(sym, 0, sym, _expr);
  Expression antideriv_expr = Expression();
  antideriv_expr._expr = antideriv.eval_integ();
  antideriv_expr._expr += C;

  // Recreate symbols list
  std::vector<GiNaC::symbol> symbols;
  GetSymbolicsFromEx(antideriv_expr._expr, symbols);
  for (size_t i = 0; i < symbols.size(); ++i) {
    GiNaC::symbol symbol = symbols[i];
    antideriv_expr._symbols[symbol.get_name()] = symbol;
    antideriv_expr._symList.append(symbol);
  }

  std::ostringstream oss;
  oss << antideriv_expr._expr;

  antideriv_expr._userString = oss.str();

  return std::make_unique<Expression>(antideriv_expr);
}

std::optional<double>
Expression::CalculateIntegral(const std::string& variable,
                              double lowerBound,
                              double upperBound)
{
  if (!CheckSymbolName(variable))
    return std::nullopt;

  GiNaC::symbol sym = _symbols[variable];

  GiNaC::ex res;

  try {
    res = adaptivesimpson(sym, lowerBound, upperBound, _expr).evalf();
  } catch (const std::exception& ex) {
    _error = ex.what();
    return std::nullopt;
  }

  if (GiNaC::is_a<GiNaC::numeric>(res)) {
    GiNaC::numeric numeric_res = GiNaC::ex_to<GiNaC::numeric>(res);
    if (numeric_res.is_real())
      return std::make_optional(numeric_res.to_double());
  }

  // If result is not real number, return nullopt
  _error = "result of numeric integration is not a real number";

  return std::nullopt;
}