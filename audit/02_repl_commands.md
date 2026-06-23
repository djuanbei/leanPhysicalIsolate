# Pantograph REPL command set

## Command dispatch (Repl.lean)
      | "tactic" => do -- Exit from the current fragment
      | "conv" => do
      | "calc" => do
  | "reset"         => run reset
  | "stat"          => run stat
  | "options.set"   => run options_set
  | "options.print" => run options_print
  | "expr.echo"     => run expr_echo
  | "env.describe"  => run env_describe
  | "env.module_read" => run env_module_read
  | "env.catalog"   => run env_catalog
  | "env.inspect"   => run env_inspect
  | "env.add"       => run env_add
  | "env.save"      => run env_save
  | "env.load"      => run env_load
  | "env.parse"     => run env_parse
  | "goal.start"    => run goal_start
  | "goal.tactic"   => run goal_tactic
  | "goal.continue" => run goal_continue
  | "goal.subsume" => run goal_subsume
  | "goal.delete"   => run goal_delete
  | "goal.print"    => run goal_print
  | "goal.save"     => run goal_save
  | "goal.load"     => run goal_load
  | "frontend.process" => run frontend_process
  | "frontend.distil"  => run frontend_distil
  | "frontend.track"   => run frontend_track
  | "frontend.refactor" => run frontend_refactor

## Total commands:
28
