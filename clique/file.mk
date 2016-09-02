TARGET := solve_max_common_subgraph

SOURCES := \
    solve_max_common_subgraph.cc

TGT_LDLIBS := $(boost_ldlibs) -lmax_common_subgraph
TGT_LDFLAGS := -L${TARGET_DIR}
TGT_PREREQS := libmax_common_subgraph.a

