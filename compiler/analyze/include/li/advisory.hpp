#pragma once

#include "li/ast.hpp"
#include "li/diagnostics.hpp"

#include <string>

namespace li {

// Non-fatal advisory diagnostics (warnings/notes) surfaced by the agent check
// path (`lic check --format=json` / `lic diagnose`). They never change the
// accept/reject verdict on their own: warnings only escalate under
// --deny-warnings. Re-landed on the current parity AST (c132e1a9 removed the
// original advisory.cpp, which referenced pre-squash AST fields).
void run_advisory_passes(const Module& module, const std::string& file,
                         DiagnosticBag& diags);

}  // namespace li