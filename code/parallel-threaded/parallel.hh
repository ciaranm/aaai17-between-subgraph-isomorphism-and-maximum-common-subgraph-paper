/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef CODE_GUARD_PARALLEL_HH
#define CODE_GUARD_PARALLEL_HH 1

#include "graph.hh"
#include "params.hh"
#include "result.hh"

auto parallel_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const Params & params) -> Result;

auto parallel_ix_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const Params & params) -> Result;

#endif
