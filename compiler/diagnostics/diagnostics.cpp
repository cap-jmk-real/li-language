#include "li/diagnostics.hpp"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>

namespace li {
namespace {

std::string json_escape(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  out.push_back('"');
  for (const char ch : text) {
    switch (ch) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(ch) < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(ch));
          out += buf;
        } else {
          out.push_back(ch);
        }
    }
  }
  out.push_back('"');
  return out;
}

// Map a diagnostic message to a stable error code when none was attached at
// the emission site. Mirrors the pre-squash agent CLI (c132e1a9 removed it).
std::string infer_diagnostic_code(std::string_view message) {
  const auto has = [&](std::string_view needle) {
    return message.find(needle) != std::string_view::npos;
  };
  if (has("indentation")) {
    return "E0101";
  }
  if (has("out of range") || has("index")) {
    return "E0201";
  }
  if (has("type") && has("expected")) {
    return "E0202";
  }
  if (has("import") || has("module")) {
    return "resolve.import";
  }
  if (has("stdlib_symbol_shadow")) {
    return "E0330";
  }
  if (has("parallel_requires_disjoint") || has("disjoint slices")) {
    return "E0320";
  }
  if (has("missing requires")) {
    return "E0301";
  }
  if (has("missing ensures") || has("missing ensures clause")) {
    return "E0302";
  }
  if (has("ensures true") || has("weak postcondition") ||
      has("postcondition is false")) {
    return "E0303";
  }
  if (has("does not satisfy callee") || has("violates its requires")) {
    return "E0304";
  }
  if (has("refinement")) {
    return "E0305";
  }
  if (has("borrow")) {
    return "E0310";
  }
  return "lic.error";
}

// Stable category strings for agents (diagnostic-v1); maps E-codes to
// semantic ids the JSON smoke asserts on (type.index, ...).
std::string agent_diagnostic_code(std::string_view code) {
  if (code == "E0201") {
    return "type.index";
  }
  if (code == "E0202") {
    return "type.mismatch";
  }
  if (code == "E0101") {
    return "parse.indent";
  }
  if (code == "E0301") {
    return "contract.requires";
  }
  if (code == "E0302") {
    return "contract.ensures";
  }
  if (code == "E0303") {
    return "contract.weak_ensures";
  }
  if (code == "E0304") {
    return "contract.callee_requires";
  }
  if (code == "E0305") {
    return "type.refinement";
  }
  if (code == "E0320") {
    return "parallel.disjoint";
  }
  if (code == "E0330") {
    return "resolve.shadow";
  }
  return std::string(code);
}

std::string effective_code(const Diagnostic& d) {
  if (!d.code.empty()) {
    return d.code;
  }
  return infer_diagnostic_code(d.message);
}

std::string_view severity_string(DiagnosticSeverity severity) {
  switch (severity) {
    case DiagnosticSeverity::Error:
      return "error";
    case DiagnosticSeverity::Warning:
      return "warning";
    case DiagnosticSeverity::Note:
      return "note";
  }
  return "error";
}

std::string_view severity_label(DiagnosticSeverity severity) {
  switch (severity) {
    case DiagnosticSeverity::Error:
      return "error";
    case DiagnosticSeverity::Warning:
      return "warning";
    case DiagnosticSeverity::Note:
      return "note";
  }
  return "error";
}

}  // namespace

void DiagnosticBag::error(SourceLoc loc, std::string message) {
  push(DiagnosticSeverity::Error, std::move(loc), {}, std::move(message), {});
}

void DiagnosticBag::error(SourceLoc loc, std::string code, std::string message,
                          std::string hint) {
  push(DiagnosticSeverity::Error, std::move(loc), std::move(code), std::move(message),
       std::move(hint));
}

void DiagnosticBag::warning(SourceLoc loc, std::string code, std::string message,
                            std::string hint) {
  push(DiagnosticSeverity::Warning, std::move(loc), std::move(code), std::move(message),
       std::move(hint));
}

void DiagnosticBag::note(SourceLoc loc, std::string code, std::string message,
                         std::string hint) {
  push(DiagnosticSeverity::Note, std::move(loc), std::move(code), std::move(message),
       std::move(hint));
}

void DiagnosticBag::push(DiagnosticSeverity severity, SourceLoc loc, std::string code,
                         std::string message, std::string hint) {
  std::optional<std::string> hint_opt;
  if (!hint.empty()) {
    hint_opt = std::move(hint);
  }
  items_.push_back(
      Diagnostic{std::move(loc), std::move(message), severity, std::move(code), std::move(hint_opt)});
}

bool DiagnosticBag::has_errors() const {
  return std::any_of(items_.begin(), items_.end(), [](const Diagnostic& d) {
    return d.severity == DiagnosticSeverity::Error;
  });
}

bool DiagnosticBag::has_warnings() const {
  return std::any_of(items_.begin(), items_.end(), [](const Diagnostic& d) {
    return d.severity == DiagnosticSeverity::Warning;
  });
}

void print_diagnostics(const DiagnosticBag& bag) {
  for (const auto& d : bag.items()) {
    std::cerr << d.loc.file << ':' << d.loc.line << ':' << d.loc.column << ": "
              << severity_label(d.severity) << ": " << d.message << '\n';
  }
}

void render_diagnostics(const DiagnosticBag& bag, std::ostream& out) {
  for (const auto& d : bag.items()) {
    out << d.loc.file << ':' << d.loc.line << ':' << d.loc.column << ": "
        << severity_label(d.severity) << ": " << d.message << '\n';
    if (d.hint && !d.hint->empty()) {
      out << "  hint: " << *d.hint << '\n';
    }
  }
}

void print_diagnostics_json(const DiagnosticBag& bag, std::ostream& out,
                            std::string_view command) {
  out << "{\"version\":1,\"schema\":\"diagnostic-v1\",\"tool\":\"lic\",\"command\":"
      << json_escape(command) << ",\"ok\":" << (bag.has_errors() ? "false" : "true")
      << ",\"diagnostics\":[";
  bool first = true;
  for (const auto& d : bag.items()) {
    if (!first) {
      out << ',';
    }
    first = false;
    const std::string code = agent_diagnostic_code(effective_code(d));
    out << "{\"severity\":" << json_escape(severity_string(d.severity)) << ",\"file\":"
        << json_escape(d.loc.file) << ",\"line\":" << d.loc.line << ",\"column\":" << d.loc.column
        << ",\"offset\":" << d.loc.offset << ",\"code\":" << json_escape(code)
        << ",\"message\":" << json_escape(d.message);
    if (d.hint && !d.hint->empty()) {
      out << ",\"fix_hint\":" << json_escape(*d.hint);
    } else {
      out << ",\"fix_hint\":null";
    }
    out << '}';
  }
  out << "]}\n";
}

}  // namespace li