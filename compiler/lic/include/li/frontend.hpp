#pragma once

#include "li/ast.hpp"
#include "li/diagnostics.hpp"

#include <optional>
#include <string>

namespace li {

// Walker-parity front end: source policies -> parse -> transitive import
// merge (walker order) -> typecheck of the main module's own procs. Shared by
// the parity `lic check`/`build`/`mir`/`verify` path and the agent CLI
// (`lic check --format=json`, `lic diagnose`).
bool frontend(const char* path, const std::string& source, Module& out,
              DiagnosticBag& diags);

// Resolve `import <module>` from `base_file` using the walker's candidate
// chain (same-directory, workspace packages/, std.* tree, same-package
// li.toml self-import). Mirrors li_rt_resolve_import in runtime/li_rt.c.
std::optional<std::string> resolve_import_path(const std::string& module,
                                               const std::string& base_file);

}  // namespace li