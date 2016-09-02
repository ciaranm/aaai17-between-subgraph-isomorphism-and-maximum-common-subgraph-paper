/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "sequential.hh"

#include <algorithm>
#include <chrono>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <numeric>
#include <random>
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
using std::mt19937;
using std::numeric_limits;
using std::pair;
using std::to_string;
using std::tuple;
using std::uniform_int_distribution;
using std::unique;
using std::vector;

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

using std::cerr;
using std::endl;

namespace
{
    /// We'll use an array of unsigned long longs to represent our bits.
    using BitWord = unsigned long long;

    /// Number of bits per word.
    static const constexpr int bits_per_word = sizeof(BitWord) * 8;

    /**
     * A bitset with a fixed maximum size. This only provides the operations
     * we actually use in the bitset algorithms: it's more readable this way
     * than doing all the bit voodoo inline.
     *
     * Indices start at 0.
     */
    template <unsigned words_>
    class FixedBitSet
    {
        private:
            using Bits = std::array<BitWord, words_>;

            Bits _bits = {{ }};

        public:
            FixedBitSet() = default;

            FixedBitSet(unsigned size)
            {
                assert(size <= words_ * bits_per_word);
            }

            FixedBitSet(const FixedBitSet &) = default;

            FixedBitSet & operator= (const FixedBitSet &) = default;

            /**
             * Set a given bit 'on'.
             */
            auto set(int a) -> void
            {
                // The 1 does have to be of type BitWord. If we just specify a
                // literal, it ends up being an int, and it isn't converted
                // upwards until after the shift is done.
                _bits[a / bits_per_word] |= (BitWord{ 1 } << (a % bits_per_word));
            }

            /**
             * Set a given bit 'off'.
             */
            auto reset(int a) -> void
            {
                _bits[a / bits_per_word] &= ~(BitWord{ 1 } << (a % bits_per_word));
            }

            auto reset() -> void
            {
                for (auto & p : _bits)
                    p = 0;
            }

            /**
             * Is a given bit on?
             */
            auto operator[] (int a) const -> bool
            {
                return _bits[a / bits_per_word] & (BitWord{ 1 } << (a % bits_per_word));
            }

            /**
             * How many bits are on?
             */
            auto count() const -> unsigned
            {
                unsigned result = 0;
                for (auto & p : _bits)
                    result += __builtin_popcountll(p);
                return result;
            }

            /**
             * Are any bits on?
             */
            auto none() const -> bool
            {
                for (auto & p : _bits)
                    if (0 != p)
                        return false;
                return true;
            }

            /**
             * Intersect (bitwise-and) with another set.
             */
            auto operator&= (const FixedBitSet<words_> & other) -> FixedBitSet &
            {
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    _bits[i] = _bits[i] & other._bits[i];
                return *this;
            }

            auto operator& (const FixedBitSet & other) const -> FixedBitSet
            {
                FixedBitSet result;
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    result._bits[i] = _bits[i] & other._bits[i];
                return result;
            }

            /**
             * Union (bitwise-or) with another set.
             */
            auto operator|= (const FixedBitSet<words_> & other) -> FixedBitSet &
            {
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    _bits[i] = _bits[i] | other._bits[i];
                return *this;
            }

            auto operator| (const FixedBitSet & other) const -> FixedBitSet
            {
                FixedBitSet result;
                for (typename Bits::size_type i = 0 ; i < words_ ; ++i)
                    result._bits[i] = _bits[i] | other._bits[i];
                return result;
            }

            static const constexpr unsigned npos = numeric_limits<unsigned>::max();

            /**
             * Return the index of the first set ('on') bit, or -1 if we are
             * empty.
             */
            auto find_first() const -> unsigned
            {
                for (typename Bits::size_type i = 0 ; i < _bits.size() ; ++i) {
                    int b = __builtin_ffsll(_bits[i]);
                    if (0 != b)
                        return i * bits_per_word + b - 1;
                }
                return npos;
            }

            auto find_next(unsigned start_after) const -> unsigned
            {
                unsigned start = start_after + 1;

                auto word = _bits[start / bits_per_word];
                word &= ~((BitWord(1) << (start % bits_per_word)) - 1);
                int b = __builtin_ffsll(word);
                if (0 != b)
                    return start / bits_per_word * bits_per_word + b - 1;

                for (typename Bits::size_type i = start / bits_per_word + 1; i < _bits.size() ; ++i) {
                    int b = __builtin_ffsll(_bits[i]);
                    if (0 != b)
                        return i * bits_per_word + b - 1;
                }

                return npos;
            }

            auto operator== (const FixedBitSet<words_> & other) const -> bool
            {
                if (_bits.size() != other._bits.size())
                    return false;

                for (typename Bits::size_type i = 0 ; i < _bits.size() ; ++i)
                    if (_bits[i] != other._bits[i])
                        return false;

                return true;
            }

            auto operator~ () const -> FixedBitSet
            {
                FixedBitSet result = *this;
                for (auto & p : result._bits)
                    p = ~p;
                return result;
            }
    };

    template <typename Bitset_>
    struct SIP
    {
        struct Domain
        {
            unsigned v;
            bool fixed;
            Bitset_ values;
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

        const Params & params;
        unsigned domain_size;

        Result result;

        list<pair<vector<Bitset_>, vector<Bitset_> > > adjacency_constraints;
        vector<unsigned> pattern_degrees, target_degrees;

        Domains initial_domains;

        unsigned wildcard_start;
        Bitset_ all_wildcards;

        SIP(const Params & k, const Graph & pattern, const Graph & target) :
            params(k),
            domain_size(target.size() + params.except),
            pattern_degrees(pattern.size()),
            target_degrees(domain_size),
            initial_domains(pattern.size()),
            wildcard_start(target.size()),
            all_wildcards(domain_size)
        {
            for (unsigned v = wildcard_start ; v != domain_size ; ++v)
                all_wildcards.set(v);

            // build up adjacency bitsets
            add_adjacency_constraints(pattern, target);

            for (unsigned p = 0 ; p < pattern.size() ; ++p)
                pattern_degrees[p] = pattern.degree(p);

            for (unsigned t = 0 ; t < target.size() ; ++t)
                target_degrees[t] = target.degree(t);

            if (params.except >= 1)
                for (unsigned v = 0 ; v < params.except ; ++v)
                    target_degrees.at(wildcard_start + v) = params.high_wildcards ? target.size() + 1 : 0;

            vector<vector<vector<unsigned> > > p_nds(params.cnds ? adjacency_constraints.size() * adjacency_constraints.size() : adjacency_constraints.size());
            vector<vector<vector<unsigned> > > t_nds(params.cnds ? adjacency_constraints.size() * adjacency_constraints.size() : adjacency_constraints.size());

            if (params.cnds) {
                for (unsigned p = 0 ; p < pattern.size() ; ++p) {
                    unsigned cn = 0;
                    for (auto & c : adjacency_constraints) {
                        for (auto & d : adjacency_constraints) {
                            p_nds[cn].resize(pattern.size());
                            for (unsigned q = 0 ; q < pattern.size() ; ++q)
                                if (c.first[p][q])
                                    p_nds[cn][p].push_back(d.first[q].count());
                            sort(p_nds[cn][p].begin(), p_nds[cn][p].end(), greater<unsigned>());
                            ++cn;
                        }
                    }
                }

                for (unsigned t = 0 ; t < target.size() ; ++t) {
                    unsigned cn = 0;
                    for (auto & c : adjacency_constraints) {
                        for (auto & d : adjacency_constraints) {
                            t_nds[cn].resize(target.size());
                            for (unsigned q = 0 ; q < target.size() ; ++q)
                                if (c.second[t][q])
                                    t_nds[cn][t].push_back(d.second[q].count());
                            sort(t_nds[cn][t].begin(), t_nds[cn][t].end(), greater<unsigned>());
                            ++cn;
                        }
                    }
                }
            }
            else if (params.nds) {
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
                initial_domains[p].values = Bitset_(domain_size);
                initial_domains[p].fixed = false;

                // decide initial domain values
                for (unsigned t = 0 ; t < target.size() ; ++t) {
                    bool ok = true;

                    for (auto & c : adjacency_constraints) {
                        // check loops
                        if (c.first[p][p] && ! c.second[t][t])
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
                        for (unsigned cn = 0 ; cn < 1 && ok ; ++cn) {
                            for (unsigned i = params.except ; i < p_nds[cn][p].size() ; ++i) {
                                if (t_nds[cn][t][i - params.except] + params.except < p_nds[cn][p][i]) {
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

            if (params.expensive_stats) {
                Bitset_ initial_domains_union = Bitset_(domain_size);
                for (unsigned p = 0 ; p < pattern.size() ; ++p)
                    initial_domains_union |= initial_domains[p].values;

                result.stats.emplace("UA", to_string(initial_domains_union.count()));
                result.stats.emplace("UP", to_string(domain_size));

                vector<pair<Domain *, unsigned> > assignments;

                for (auto & d : initial_domains)
                    for (auto v = d.values.find_first() ; v != Bitset_::npos && v < wildcard_start ; v = d.values.find_next(v))
                        assignments.emplace_back(&d, v);

                unsigned long long pairs_seen = 0, pairs_disallowed = 0;
                if (assignments.size() >= 2) {
                    uniform_int_distribution<typename decltype(assignments)::size_type>
                        all_dist(0, assignments.size() - 1), all_but_first_dist(1, assignments.size() - 1);
                    mt19937 rand(666);
                    for (unsigned n = 0 ; n < 1000000 ; ++n) {
                        swap(assignments[0], assignments[all_dist(rand)]);
                        swap(assignments[1], assignments[all_but_first_dist(rand)]);
                        auto & a = assignments[0], & b = assignments[1];
                        if (a.first->v != b.first->v && a.second != b.second) {
                            ++pairs_seen;
                            bool disallowed = false;
                            for (auto & c : adjacency_constraints)
                                if (c.first[a.first->v][b.first->v] && ! c.second[a.second][b.second])
                                    disallowed = true;

                            if (disallowed)
                                ++pairs_disallowed;
                        }
                    }
                }

                result.stats.emplace("PS", to_string(pairs_seen));
                result.stats.emplace("PD", to_string(pairs_disallowed));
            }
        }

        auto add_complement_constraints(const Graph & pattern, const Graph & target) -> auto
        {
            auto & d1 = *adjacency_constraints.insert(
                    adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
            build_d1_adjacency(pattern, false, d1.first, true);
            build_d1_adjacency(target, true, d1.second, true);

            return d1;
        }

        auto add_adjacency_constraints(const Graph & pattern, const Graph & target) -> void
        {
            auto & d1 = *adjacency_constraints.insert(
                    adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
            build_d1_adjacency(pattern, false, d1.first, false);
            build_d1_adjacency(target, true, d1.second, false);

            if (params.d2graphs) {
                auto & d21 = *adjacency_constraints.insert(
                        adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
                auto & d22 = *adjacency_constraints.insert(
                        adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
                auto & d23 = *adjacency_constraints.insert(
                        adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));

                build_d2_adjacency(pattern.size(), d1.first, false, d21.first, d22.first, d23.first);
                build_d2_adjacency(target.size(), d1.second, true, d21.second, d22.second, d23.second);
            }

            if (params.induced) {
                auto d1c = add_complement_constraints(pattern, target);

                if (params.d2cgraphs) {
                    auto & d21c = *adjacency_constraints.insert(
                            adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
                    auto & d22c = *adjacency_constraints.insert(
                            adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));
                    auto & d23c = *adjacency_constraints.insert(
                            adjacency_constraints.end(), make_pair(vector<Bitset_>(), vector<Bitset_>()));

                    build_d2_adjacency(pattern.size(), d1c.first, false, d21c.first, d22c.first, d23c.first);
                    build_d2_adjacency(target.size(), d1c.second, true, d21c.second, d22c.second, d23c.second);
                }
            }
        }

        auto build_d1_adjacency(const Graph & graph, bool is_target, vector<Bitset_> & adj, bool complement) const -> void
        {
            adj.resize(graph.size());
            for (unsigned t = 0 ; t < graph.size() ; ++t) {
                adj[t] = Bitset_(is_target ? domain_size : graph.size());
                for (unsigned u = 0 ; u < graph.size() ; ++u)
                    if (graph.adjacent(t, u) != complement)
                        adj[t].set(u);
            }
        }

        auto build_d2_adjacency(
                const unsigned graph_size,
                const vector<Bitset_> & d1_adj,
                bool is_target,
                vector<Bitset_> & adj1,
                vector<Bitset_> & adj2,
                vector<Bitset_> & adj3) const -> void
        {
            adj1.resize(graph_size);
            adj2.resize(graph_size);
            adj3.resize(graph_size);

            vector<vector<unsigned> > counts(graph_size, vector<unsigned>(graph_size, 0));

            for (unsigned t = 0 ; t < graph_size ; ++t) {
                adj1[t] = Bitset_(is_target ? domain_size : graph_size);
                adj2[t] = Bitset_(is_target ? domain_size : graph_size);
                adj3[t] = Bitset_(is_target ? domain_size : graph_size);
                for (auto u = d1_adj[t].find_first() ; u != Bitset_::npos ; u = d1_adj[t].find_next(u))
                    if (t != u)
                        for (auto v = d1_adj[u].find_first() ; v != Bitset_::npos ; v = d1_adj[u].find_next(v))
                            if (u != v && t != v)
                                ++counts[t][v];
            }

            for (unsigned t = 0 ; t < graph_size ; ++t)
                for (unsigned u = 0 ; u < graph_size ; ++u) {
                    if (counts[t][u] >= 3 + (is_target ? 0 : params.except))
                        adj3[t].set(u);
                    if (counts[t][u] >= 2 + (is_target ? 0 : params.except))
                        adj2[t].set(u);
                    if (counts[t][u] >= 1 + (is_target ? 0 : params.except))
                        adj1[t].set(u);
                }
        }

        auto select_branch_domain(Domains & domains) -> typename Domains::iterator
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

        auto select_unit_domain(Domains & domains) -> typename Domains::iterator
        {
            return find_if(domains.begin(), domains.end(), [&] (const auto & a) {
                    if (! a.fixed) {
                        auto c = a.values.count();
                        return 1 == c || (c > 1 && a.values.find_first() >= wildcard_start);
                    }
                    else
                        return false;
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
                            if (c.first[unit_domain_v][d.v])
                                d.values &= (c.second[unit_domain_value] | all_wildcards);

                    if (d.values.none())
                        return false;
                }
            }

            return true;
        }

        auto cheap_all_different(Domains & domains) -> bool
        {
            // pick domains smallest first, with tiebreaking
            vector<pair<int, int> > domains_order;
            domains_order.resize(domains.size());
            for (unsigned d = 0 ; d < domains.size() ; ++d) {
                domains_order[d].first = d;
                domains_order[d].second = domains[d].values.count();
            }

            sort(domains_order.begin(), domains_order.begin() + domains.size(),
                    [&] (const pair<int, int> & a, const pair<int, int> & b) {
                        return a.second < b.second || (a.second == b.second && a.first < b.first);
                    });

            // counting all-different
            Bitset_ domains_so_far = Bitset_(domain_size), hall = Bitset_(domain_size);
            unsigned neighbours_so_far = 0;

            for (int i = 0, i_end = domains.size() ; i != i_end ; ++i) {
                auto & d = domains.at(domains_order.at(i).first);

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
                    branch_value != Bitset_::npos ;
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
                        Bitset_ just_branch_value = d.values;
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
            auto wildcards = Bitset_(domain_size);
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
                if (params.expensive_stats)
                    record_domain_sizes_in_stats(initial_domains);
                solve(initial_domains, assignments);
            }
            else if (params.expensive_stats)
                record_domain_sizes_in_stats(initial_domains);
        }
    };
}

auto sequential_subgraph_isomorphism(const pair<Graph, Graph> & graphs, const Params & params) -> Result
{
    if (graphs.second.size() + params.except <= 63) {
        SIP<FixedBitSet<64 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 127) {
        SIP<FixedBitSet<128 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 255) {
        SIP<FixedBitSet<256 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 447) {
        SIP<FixedBitSet<448 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else if (graphs.second.size() + params.except <= 511) {
        SIP<FixedBitSet<512 / sizeof(unsigned long long)> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
    else {
        SIP<boost::dynamic_bitset<> > sip(params, graphs.first, graphs.second);
        sip.run();
        return sip.result;
    }
}

auto sequential_ix_subgraph_isomorphism(const pair<Graph, Graph> & graphs, const Params & params) -> Result
{
    auto modified_params = params;
    Result modified_result;

    while (! *modified_params.abort) {
        auto start_time = steady_clock::now();

        SIP<boost::dynamic_bitset<> > sip(modified_params, graphs.first, graphs.second);

        sip.run();

        auto pass_time = duration_cast<milliseconds>(steady_clock::now() - start_time);
        modified_result.times.push_back(pass_time);

        modified_result.nodes += sip.result.nodes;
        if (! sip.result.isomorphism.empty()) {
            modified_result.isomorphism = sip.result.isomorphism;
            modified_result.stats.emplace("EXCEPT", to_string(modified_params.except));
            modified_result.stats.emplace("SIZE", to_string(graphs.first.size() - modified_params.except));
            return modified_result;
        }
        else
            modified_result.stats.emplace("FAIL" + to_string(modified_params.except), to_string(pass_time.count()));

        if (++modified_params.except >= graphs.first.size())
            break;
    }

    return modified_result;
}

