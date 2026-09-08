#pragma once

#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace li {

struct SourceLoc {
  std::string file;
  std::size_t line = 1;
  std::size_t column = 1;
  std::size_t offset = 0;
};

enum class DiagnosticSeverity { Error, Warning, Note };

struct Diagnostic {
  SourceLoc loc;
  std::string message;
  DiagnosticSeverity severity = DiagnosticSeverity::Error;
  std::string code;
  std::optional<std::string> hint;
};

class DiagnosticBag {
 public:
  void error(SourceLoc loc, std::string message);
  void error(SourceLoc loc, std::string code, std::string message, std::string hint);
  void warning(SourceLoc loc, std::string code, std::string message, std::string hint);
  void note(SourceLoc loc, std::string code, std::string message, std::string hint);
  bool empty() const { return items_.empty(); }
  bool has_errors() const;
  bool has_warnings() const;
  const std::vector<Diagnostic>& items() const { return items_; }

 private:
  void push(DiagnosticSeverity severity, SourceLoc loc, std::string code,
            std::string message, std::string hint);
  std::vector<Diagnostic> items_;
};

void print_diagnostics(const DiagnosticBag& bag);

// Agent-facing JSON envelope (diagnostic-v1): emitted by `lic check
// --format=json` and `lic diagnose`. Codes are stable semantic ids
// (type.index, contract.weak_ensures, ...) so agents can act on them.
void print_diagnostics_json(const DiagnosticBag& bag, std::ostream& out,
                            std::string_view command);

// Human rendering to a stream (warnings/notes labelled), used by the agent
// check path; parity `lic check` keeps print_diagnostics' exact format.
void render_diagnostics(const DiagnosticBag& bag, std::ostream& out);

}  // namespace li