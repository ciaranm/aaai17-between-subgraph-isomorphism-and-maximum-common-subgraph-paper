/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "sequential.hh"

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

#include <iostream>

#include <boost/dynamic_bitset.hpp>

using std::find_if;
using std::get;
using std::greater;
using std::list;
using std::make_pair;
using std::map;
using std::pair;
using std::to_string;
using std::tuple;
using std::unique;
using std::vector;

using std::cerr;
using std::endl;

using bitset = boost::dynamic_bitset<>;

namespace
{
    struct Domain
    {
        unsigned v;
        bool fixed;
        bitset values;
    };

    using Domains = vector<Domain>;

    struct Assignments
    {
        vector<tuple<unsigned, unsigned, bool> > trail;

        auto push_branch(unsigned a, unsigned b) -> void
        {
            trail.emplace_back(a, b, true);
        }

        auto push_implication(unsigned a, unsigned b) -> void
        {
            if (trail.end() == find_if(trail.begin(), trail.end(), [&] (const auto & x) {
                        return get<0>(x) == a && get<1>(x) == b;
                        }))
                trail.emplace_back(a, b, false);
        }

        auto pop() -> void
        {
            while ((! trail.empty()) && (! get<2>(trail.back())))
                trail.pop_back();

            if (! trail.empty())
                trail.pop_back();
        }

        auto store_to(map<int, int> & m, unsigned wildcard_start) -> void
        {
            for (auto & t : trail) {
                if (get<1>(t) >= wildcard_start)
                    m.emplace(get<0>(t), -1);
                else
                    m.emplace(get<0>(t), get<1>(t));
            }
        }
    };

    struct SIP
    {
        const Params & params;
        unsigned domain_size;

        Result result;

        list<pair<vector<bitset>, vector<bitset> > > adjacency_constraints;
        vector<unsigned> pattern_degrees, target_degrees;

        Domains initial_domains;

        unsigned wildcard_start;

        SIP(const Params & k, const Graph & pattern, const Graph & target) :
            params(k),
            domain_size(target.size() + params.except),
            pattern_degrees(pattern.size()),
            target_degrees(domain_size),
            initial_domains(pattern.size()),
            wildcard_start(target.size())
        {
            // build up distance 1 adjacency bitsets
            add_adjacency_constraints(pattern, target);

            for (unsigned p = 0 ; p < pattern.size() ; ++p)
                pattern_degrees[p] = pattern.degree(p);

            for (unsigned t = 0 ; t < target.size() ; ++t)
                target_degrees[t] = target.degree(t);

            if (params.except >= 1)
                for (unsigned v = 0 ; v < params.except ; ++v)
                    target_degrees.at(wildcard_start + v) = 0;

            vector<vector<vector<unsigned> > > p_nds(adjacency_constraints.size());
            vector<vector<vector<unsigned> > > t_nds(adjacency_constraints.size());

            if (params.nds) {
                for (unsigned p = 0 ; p < pattern.size() ; ++p) {
                    unsigned cn = 0;
                    for (auto & c : adjacency_constraints) {
                        p_nds[cn].resize(pattern.size());
                        for (unsigned q = 0 ; q < pattern.size() ; ++q)
                            if (c.first[p][q])
                                p_nds[cn][p].push_back(c.first[q].count());
                        sort(p_nds[cn][p].begin(), p_nds[cn][p].end(), greater<unsigned>());
                        ++cn;
                    }
                }

                for (unsigned t = 0 ; t < target.size() ; ++t) {
                    unsigned cn = 0;
                    for (auto & c : adjacency_constraints) {
                        t_nds[cn].resize(target.size());
                        for (unsigned q = 0 ; q < target.size() ; ++q)
                            if (c.second[t][q])
                                t_nds[cn][t].push_back(c.second[q].count());
                        sort(t_nds[cn][t].begin(), t_nds[cn][t].end(), greater<unsigned>());
                        ++cn;
                    }
                }
            }

            // build up initial domains
            for (unsigned p = 0 ; p < pattern.size() ; ++p) {
                initial_domains[p].v = p;
                initial_domains[p].values = bitset(domain_size);
                initial_domains[p].fixed = false;

                // decide initial domain values
                for (unsigned t = 0 ; t < target.size() ; ++t) {
                    bool ok = true;

                    for (auto & c : adjacency_constraints) {
                        // check loops
                        if (c.first[p][p] && ! c.second[t][t])
                            ok = false;
                        else if (params.induced && ! c.first[p][p] && c.second[t][t])
                            ok = false;

                        // check degree
                        if (ok && params.degree && 0 == params.except && ! (c.first[p].count() <= c.second[t].count()))
                            ok = false;

                        // check except-degree
                        if (ok && params.degree && params.except >= 1 && ! (c.first[p].count() <= c.second[t].count() + params.except))
                            ok = false;

                        if (! ok)
                            break;
                    }

                    // neighbourhood degree sequences
                    if (params.nds) {
                        for (unsigned cn = 0 ; cn < adjacency_constraints.size() && ok ; ++cn) {
                            for (unsigned i = params.except ; i < p_nds[cn][p].size() ; ++i) {
                                if (t_nds[cn][t][i - params.except] < p_nds[cn][p][i]) {
                                    ok = false;
                                    break;
                                }
                            }
                        }
                    }

                    if (ok)
                        initial_domains[p].values.set(t);
                }

                // wildcard in domain?
                if (params.except >= 1)
                    for (unsigned v = wildcard_start ; v != domain_size ; ++v)
                        initial_domains[p].values.set(v);
            }
        }

        auto add_adjacency_constraints(const Graph & pattern, const Graph & target) -> void
        {
            auto & d1 = *adjacency_constraints.insert(
                    adjacency_constraints.end(), make_pair(vector<bitset>(), vector<bitset>()));
            build_d1_adjacency(pattern, false, d1.first);
            build_d1_adjacency(target, true, d1.second);

            if (params.d2graphs) {
                auto & d21 = *adjacency_constraints.insert(
                        adjacency_constraints.end(), make_pair(vector<bitset>(), vector<bitset>()));
                auto & d22 = *adjacency_constraints.insert(
                        adjacency_constraints.end(), make_pair(vector<bitset>(), vector<bitset>()));
                auto & d23 = *adjacency_constraints.insert(
                        adjacency_constraints.end(), make_pair(vector<bitset>(), vector<bitset>()));

                build_d2_adjacency(pattern, false, d21.first, d22.first, d23.first);
                build_d2_adjacency(target, true, d21.second, d22.second, d23.second);
            }
        }

        auto build_d1_adjacency(const Graph & graph, bool is_target, vector<bitset> & adj) const -> void
        {
            adj.resize(graph.size());
            for (unsigned t = 0 ; t < graph.size() ; ++t) {
                adj[t] = bitset(is_target ? domain_size : graph.size(), 0);
                for (unsigned u = 0 ; u < graph.size() ; ++u)
                    if (graph.adjacent(t, u))
                        adj[t].set(u);
            }
        }

        auto build_d2_adjacency(const Graph & graph,
                bool is_target,
                vector<bitset> & adj1,
                vector<bitset> & adj2,
                vector<bitset> & adj3) const -> void
        {
            adj1.resize(graph.size());
            adj2.resize(graph.size());
            adj3.resize(graph.size());
            for (unsigned t = 0 ; t < graph.size() ; ++t) {
                adj1[t] = bitset(is_target ? domain_size : graph.size(), 0);
                adj2[t] = bitset(is_target ? domain_size : graph.size(), 0);
                adj3[t] = bitset(is_target ? domain_size : graph.size(), 0);
                for (unsigned u = 0 ; u < graph.size() ; ++u)
                    if (t != u && graph.adjacent(t, u))
                        for (unsigned v = 0 ; v < graph.size() ; ++v)
                            if (u != v && t != v && graph.adjacent(u, v)) {
                                if (adj2[t].test(v))
                                    adj3[t].set(v);
                                else if (adj1[t].test(v))
                                    adj2[t].set(v);
                                else
                                    adj1[t].set(v);
                            }
            }
        }

        auto select_branch_domain(Domains & domains) -> Domains::iterator
        {
            auto best = domains.end();

            for (auto d = domains.begin() ; d != domains.end() ; ++d) {
                if (d->fixed)
                    continue;

                if (best == domains.end())
                    best = d;
                else {
                    int best_c = best->values.count();
                    int d_c = d->values.count();

                    if (d_c < best_c)
                        best = d;
                    else if (d_c == best_c) {
                        if (pattern_degrees[d->v] > pattern_degrees[best->v])
                            best = d;
                        else if (pattern_degrees[d->v] == pattern_degrees[best->v] && d->v < best->v)
                            best = d;
                    }
                }
            }

            return best;
        }

        auto select_unit_domain(Domains & domains) -> Domains::iterator
        {
            return find_if(domains.begin(), domains.end(), [] (const auto & a) {
                    return (! a.fixed) && 1 == a.values.count();
                    });
        }

        auto unit_propagate(Domains & domains, Assignments & assignments) -> bool
        {
            while (! domains.empty()) {
                auto unit_domain_iter = select_unit_domain(domains);

                if (unit_domain_iter == domains.end()) {
                    if (! cheap_all_different(domains))
                        return false;
                    unit_domain_iter = select_unit_domain(domains);
                    if (unit_domain_iter == domains.end())
                        break;
                }

                auto unit_domain_v = unit_domain_iter->v;
                auto unit_domain_value = unit_domain_iter->values.find_first();
                unit_domain_iter->fixed = true;

                assignments.push_implication(unit_domain_v, unit_domain_value);

                for (auto & d : domains) {
                    if (d.fixed)
                        continue;

                    // injectivity
                    d.values.reset(unit_domain_value);

                    // adjacency
                    if (unit_domain_value < wildcard_start)
                        for (auto & c : adjacency_constraints)
                            if (c.first[unit_domain_v].test(d.v))
                                d.values &= c.second[unit_domain_value];
                            else if (params.induced)
                                d.values &= ~c.second[unit_domain_value];

                    if (d.values.none())
                        return false;
                }
            }

            return true;
        }

        auto cheap_all_different(Domains & domains) -> bool
        {
            // pick domains smallest first, with tiebreaking
            vector<int> domains_order;
            domains_order.resize(domains.size());
            iota(domains_order.begin(), domains_order.begin() + domains.size(), 0);

            sort(domains_order.begin(), domains_order.begin() + domains.size(),
                    [&] (int a, int b) {
                        return (domains.at(a).values.count() < domains.at(b).values.count()) || (domains.at(a).values.count() == domains.at(b).values.count() && a < b);
                        });

            // counting all-different
            bitset domains_so_far = bitset(domain_size, 0), hall = bitset(domain_size, 0);
            unsigned neighbours_so_far = 0;

            for (int i = 0, i_end = domains.size() ; i != i_end ; ++i) {
                auto & d = domains.at(domains_order.at(i));

                d.values &= ~hall;

                if (d.values.none())
                    return false;

                domains_so_far |= d.values;
                ++neighbours_so_far;

                unsigned domains_so_far_popcount = domains_so_far.count();
                if (domains_so_far_popcount < neighbours_so_far)
                    return false;
                else if (domains_so_far_popcount == neighbours_so_far) {
                    neighbours_so_far = 0;
                    hall |= domains_so_far;
                    domains_so_far.reset();
                }
            }

            return true;
        }

        auto solve(
                Domains & domains,
                Assignments & assignments) -> bool
        {
            if (*params.abort)
                return false;

            ++result.nodes;

            auto branch_domain = select_branch_domain(domains);

            if (domains.end() == branch_domain) {
                assignments.store_to(result.isomorphism, wildcard_start);
                return true;
            }

            vector<unsigned> branch_values;
            for (auto branch_value = branch_domain->values.find_first() ;
                    branch_value != bitset::npos ;
                    branch_value = branch_domain->values.find_next(branch_value))
                branch_values.push_back(branch_value);

            sort(branch_values.begin(), branch_values.end(), [&] (const auto & a, const auto & b) {
                    return target_degrees.at(a) < target_degrees.at(b) || (target_degrees.at(a) == target_degrees.at(b) && a < b);
                    });

            bool already_did_a_wildcard = false;

            for (auto & branch_value : branch_values) {
                if (*params.abort)
                    return false;

                if (already_did_a_wildcard && branch_value >= wildcard_start)
                    continue;

                if (branch_value >= wildcard_start)
                    already_did_a_wildcard = true;

                assignments.push_branch(branch_domain->v, branch_value);

                Domains new_domains;
                new_domains.reserve(domains.size());
                for (auto & d : domains) {
                    if (d.fixed)
                        continue;

                    if (d.v == branch_domain->v) {
                        bitset just_branch_value = d.values;
                        just_branch_value.reset();
                        just_branch_value.set(branch_value);
                        new_domains.emplace_back(Domain{ unsigned(d.v), false, just_branch_value });
                    }
                    else
                        new_domains.emplace_back(Domain{ unsigned(d.v), false, d.values });
                }

                if (unit_propagate(new_domains, assignments))
                    if (solve(new_domains, assignments))
                        return true;

                // restore assignments
                assignments.pop();
            }

            return false;
        }

        auto record_domain_sizes_in_stats(const Domains & domains)
        {
            auto wildcards = bitset(domain_size);
            for (unsigned v = wildcard_start ; v != domain_size ; ++v)
                wildcards.set(v);

            for (auto & d : domains)
                result.stats.emplace("IDS" + to_string(d.v), to_string((d.values & ~wildcards).count()));
        }

        auto run()
        {
            Assignments assignments;

            // eliminate isolated vertices?

            if (unit_propagate(initial_domains, assignments)) {
                record_domain_sizes_in_stats(initial_domains);
                solve(initial_domains, assignments);
            }
            else
                record_domain_sizes_in_stats(initial_domains);
        }
    };
}

auto sequential_subgraph_isomorphism(const pair<Graph, Graph> & graphs, const Params & params) -> Result
{
    SIP sip(params, graphs.first, graphs.second);

    sip.run();

    return sip.result;
}

