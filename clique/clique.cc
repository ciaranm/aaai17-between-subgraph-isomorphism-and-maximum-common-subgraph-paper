/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "clique.hh"
#include "bit_graph.hh"
#include "template_voodoo.hh"

#include <algorithm>
#include <limits>
#include <iostream>
#include <atomic>
#include <mutex>

namespace
{
    using Association = std::map<std::pair<unsigned, unsigned>, unsigned>;
    using AssociatedEdges = std::vector<std::pair<unsigned, unsigned> >;

    auto modular_product(const VFGraph & g1, const VFGraph & g2) -> std::pair<Association, AssociatedEdges>
    {
        unsigned next_vertex = 0;
        Association association;
        AssociatedEdges edges;

        for (unsigned l = 0 ; l < std::min(g1.vertices_by_label.size(), g2.vertices_by_label.size()) ; ++l)
            for (unsigned v1 : g1.vertices_by_label[l])
                for (unsigned v2 : g2.vertices_by_label[l])
                    for (unsigned m = 0 ; m < std::min(g1.vertices_by_label.size(), g2.vertices_by_label.size()) ; ++m)
                        for (unsigned w1 : g1.vertices_by_label[m])
                            for (unsigned w2 : g2.vertices_by_label[m])
                                if (v1 != w1 && v2 != w2
                                        && (g1.edges[v1][v1] == g2.edges[v2][v2])
                                        && (g1.edges[w1][w1] == g2.edges[w2][w2])
                                        && (g1.edges[v1][w1] == g2.edges[v2][w2])
                                        && (g1.edges[w1][v1] == g2.edges[w2][v2])) {

                                    if (! association.count({v1, v2}))
                                        association.insert({{v1, v2}, next_vertex++});
                                    if (! association.count({w1, w2}))
                                        association.insert({{w1, w2}, next_vertex++});

                                    edges.push_back({association.find({v1, v2})->second, association.find({w1, w2})->second});
                                }

        return { association, edges };
    }

    auto unproduct(const Association & association, unsigned v) -> std::pair<unsigned, unsigned>
    {
        for (auto & a : association)
            if (a.second == v)
                return a.first;

        throw "oops";
    }

    struct Incumbent
    {
        std::atomic<unsigned> value{ 0 };

        std::mutex mutex;
        std::vector<unsigned> c;

        void update(const std::vector<unsigned> & new_c)
        {
            while (true) {
                unsigned current_value = value;
                if (new_c.size() > current_value) {
                    unsigned new_c_size = new_c.size();
                    if (value.compare_exchange_strong(current_value, new_c_size)) {
                        std::unique_lock<std::mutex> lock(mutex);
                        c = new_c;
                        std::cerr << "-- " << new_c.size() << std::endl;
                        break;
                    }
                }
                else
                    break;
            }
        }
    };

    template <unsigned n_words_>
    struct CliqueConnectedMCS
    {
        const Association & association;
        const Params & params;

        FixedBitGraph<n_words_> graph;
        std::vector<int> order, invorder;
        Incumbent incumbent;

        std::atomic<unsigned long long> nodes;

        std::vector<FixedBitSet<n_words_> > vertices_adjacent_to_by_g1;

        std::vector<std::pair<bool, FixedBitSet<n_words_> > > unsets;

        CliqueConnectedMCS(const std::pair<Association, AssociatedEdges> & g, const Params & q, const VFGraph & g1) :
            association(g.first),
            params(q),
            order(g.first.size()),
            invorder(g.first.size()),
            nodes(0),
            vertices_adjacent_to_by_g1(g1.size),
            unsets(g.first.size())
        {
            // populate our order with every vertex initially
            std::iota(order.begin(), order.end(), 0);

            // pre-calculate degrees
            std::vector<int> degrees;
            degrees.resize(g.first.size());
            for (auto & f : g.second) {
                ++degrees[f.first];
                ++degrees[f.second];
            }

            // sort on degree
            std::sort(order.begin(), order.end(),
                    [&] (int a, int b) { return true ^ (degrees[a] < degrees[b] || (degrees[a] == degrees[b] && a > b)); });

            // re-encode graph as a bit graph
            graph.resize(g.first.size());

            for (unsigned i = 0 ; i < order.size() ; ++i)
                invorder[order[i]] = i;

            for (auto & f : g.second)
                graph.add_edge(invorder[f.first], invorder[f.second]);
        }

        auto colour_class_order(
                const FixedBitSet<n_words_> & p,
                std::array<unsigned, n_words_ * bits_per_word> & p_order,
                std::array<unsigned, n_words_ * bits_per_word> & p_bounds) -> void
        {
            FixedBitSet<n_words_> p_left = p; // not coloured yet
            unsigned colour = 0;         // current colour
            unsigned i = 0;              // position in p_bounds

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<n_words_> q = p_left;

                // while we can still give something this colour
                while (! q.empty()) {
                    // first thing we can colour
                    int v = q.first_set_bit();
                    p_left.unset(v);
                    q.unset(v);

                    // can't give anything adjacent to this the same colour
                    graph.intersect_with_row_complement(v, q);

                    // record in result
                    p_bounds[i] = colour;
                    p_order[i] = v;
                    ++i;
                }
            }
        }

        auto expand(
                std::vector<unsigned> & c,
                FixedBitSet<n_words_> & p
                ) -> void
        {
            ++nodes;

            // initial colouring
            std::array<unsigned, n_words_ * bits_per_word> p_order;
            std::array<unsigned, n_words_ * bits_per_word> p_bounds;

            colour_class_order(p, p_order, p_bounds);

            // for each v in p... (v comes later)
            for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
                // bound, timeout or early exit?
                if (c.size() + p_bounds[n] <= incumbent.value || params.abort->load())
                    return;

                auto v = p_order[n];

                // consider taking v
                c.push_back(v);

                incumbent.update(c);

                // filter p to contain vertices adjacent to v
                FixedBitSet<n_words_> new_p = p;
                graph.intersect_with_row(v, new_p);

                if (! new_p.empty())
                    expand(c, new_p);

                // now consider not taking v
                c.pop_back();
                p.unset(v);
            }
        }

        auto run() -> Result
        {
            std::vector<unsigned> c;
            c.reserve(graph.size());

            FixedBitSet<n_words_> p;
            p.set_up_to(graph.size());

            // go!
            expand(c, p);

            Result result;
            result.nodes = nodes;
            for (auto & v : incumbent.c)
                result.isomorphism.insert(unproduct(association, order[v]));

            return result;
        }
    };

    template <template <unsigned> class SGI_>
    struct Apply
    {
        template <unsigned n_words_, typename> using Type = SGI_<n_words_>;
    };
}

auto clique_subgraph_isomorphism(const std::pair<VFGraph, VFGraph> & graphs, const Params & params) -> Result
{
    auto product = modular_product(graphs.first, graphs.second);

    return select_graph_size<Apply<CliqueConnectedMCS>::template Type, Result>(AllGraphSizes(), product, params, graphs.first);
}

