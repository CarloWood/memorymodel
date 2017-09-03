#pragma once

#include "ScopeDetector.h"
#include "ast.h"

class Locks : public ScopeDetector<ast::unique_lock_decl>
{
  void left_scope(ast::unique_lock_decl const& unique_lock_decl) override;
};
