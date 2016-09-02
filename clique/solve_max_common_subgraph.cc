/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include "clique.hh"

#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include <iostream>
#include <fstream>
#include <exception>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>

namespace po = boost::program_options;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

namespace
{
    class GraphFileError :
        public std::exception
    {
        private:
            std::string _what;

        public:
            GraphFileError(const std::string & filename, const std::string & message) throw ();

            auto what() const throw () -> const char *;
    };

    GraphFileError::GraphFileError(const std::string & filename, const std::string & message) throw () :
        _what("Error reading graph file '" + filename + "': " + message)
    {
    }

    auto GraphFileError::what() const throw () -> const char *
    {
        return _what.c_str();
    }

    auto read_word(std::ifstream & infile) -> unsigned
    {
        unsigned x;
        infile >> x;
        return x;
    }

    auto read_lad(const std::string & filename) -> VFGraph
    {
        VFGraph result;

        std::ifstream infile{ filename };
        if (! infile)
            throw GraphFileError{ filename, "unable to open file" };

        result.size = read_word(infile);
        if (! infile)
            throw GraphFileError{ filename, "error reading size" };

        result.vertices_by_label.resize(1);
        result.vertex_labels.resize(result.size);

        result.edges.resize(result.size);
        for (auto & e : result.edges)
            e.resize(result.size);

        for (unsigned r = 0 ; r < result.size ; ++r) {
            result.vertex_labels.at(r) = 0;
            result.vertices_by_label.at(0).push_back(r);
        }

        for (unsigned r = 0 ; r < result.size ; ++r) {
            unsigned c_end = read_word(infile);
            if (! infile)
                throw GraphFileError{ filename, "error reading edges count" };

            for (unsigned c = 0 ; c < c_end ; ++c) {
                unsigned e = read_word(infile);

                if (e >= result.size)
                    throw GraphFileError{ filename, "edge index out of bounds" };

                result.edges[r][e] = 1;
                result.edges[e][r] = 1;
            }
        }

        std::string rest;
        if (infile >> rest)
            throw GraphFileError{ filename, "EOF not reached, next text is \"" + rest + "\"" };
        if (! infile.eof())
            throw GraphFileError{ filename, "EOF not reached" };

        return result;
    }
}

/* Helper: return a function that runs the specified algorithm, dealing
 * with timing information and timeouts. */
template <typename Result_, typename Params_, typename Data_>
auto run_this_wrapped(const std::function<Result_ (const Data_ &, const Params_ &)> & func)
    -> std::function<Result_ (const Data_ &, Params_ &, bool &, int)>
{
    return [func] (const Data_ & data, Params_ & params, bool & aborted, int timeout) -> Result_ {
        /* For a timeout, we use a thread and a timed CV. We also wake the
         * CV up if we're done, so the timeout thread can terminate. */
        std::thread timeout_thread;
        std::mutex timeout_mutex;
        std::condition_variable timeout_cv;
        std::atomic<bool> abort;
        abort.store(false);
        params.abort = &abort;
        if (0 != timeout) {
            timeout_thread = std::thread([&] {
                    auto abort_time = std::chrono::steady_clock::now() + std::chrono::seconds(timeout);
                    {
                        /* Sleep until either we've reached the time limit,
                         * or we've finished all the work. */
                        std::unique_lock<std::mutex> guard(timeout_mutex);
                        while (! abort.load()) {
                            if (std::cv_status::timeout == timeout_cv.wait_until(guard, abort_time)) {
                                /* We've woken up, and it's due to a timeout. */
                                aborted = true;
                                break;
                            }
                        }
                    }
                    abort.store(true);
                    });
        }

        /* Start the clock */
        params.start_time = std::chrono::steady_clock::now();
        auto result = func(data, params);

        /* Clean up the timeout thread */
        if (timeout_thread.joinable()) {
            {
                std::unique_lock<std::mutex> guard(timeout_mutex);
                abort.store(true);
                timeout_cv.notify_all();
            }
            timeout_thread.join();
        }

        return result;
    };
}

/* Helper: return a function that runs the specified algorithm, dealing
 * with timing information and timeouts. */
template <typename Result_, typename Params_, typename Data_>
auto run_this(Result_ func(const Data_ &, const Params_ &)) -> std::function<Result_ (const Data_ &, Params_ &, bool &, int)>
{
    return run_this_wrapped(std::function<Result_ (const Data_ &, const Params_ &)>(func));
}

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                  "Display help information")
            ("timeout",            po::value<int>(),  "Abort after this many seconds")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("pattern-file",    po::value<std::string>(), "Pattern file")
            ("target-file",     po::value<std::string>(), "Target file")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("pattern-file", 1)
            .add("target-file", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] pattern-file target-file" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No algorithm or no input file specified? Show a message and exit. */
        if (! options_vars.count("pattern-file") || ! options_vars.count("target-file")) {
            std::cout << "Usage: " << argv[0] << " [options] pattern-file target-file" << std::endl;
            return EXIT_FAILURE;
        }

        /* Figure out what our options should be. */
        Params params;

        /* Create graphs */
        auto graphs = std::make_pair(
                read_lad(options_vars["pattern-file"].as<std::string>()),
                read_lad(options_vars["target-file"].as<std::string>()));

        unsigned e1 = 0, e2 = 0;
        for (unsigned v = 0 ; v < graphs.first.size ; ++v)
            for (unsigned w = 0 ; w < graphs.first.size ; ++w)
                if (graphs.first.edges[v][w])
                    ++e1;

        for (unsigned v = 0 ; v < graphs.second.size ; ++v)
            for (unsigned w = 0 ; w < graphs.second.size ; ++w)
                if (graphs.second.edges[v][w])
                    ++e2;

        /* Do the actual run. */
        bool aborted = false;
        Result result;

        result = run_this(clique_subgraph_isomorphism)(
                graphs,
                params,
                aborted,
                options_vars.count("timeout") ? options_vars["timeout"].as<int>() : 0);

        /* Stop the clock. */
        auto overall_time = duration_cast<milliseconds>(steady_clock::now() - params.start_time);

        /* Display the results. */
        std::cout << std::boolalpha << ! result.isomorphism.empty() << " " << result.nodes;

        if (aborted)
            std::cout << " aborted";

        std::cout << " " << result.isomorphism.size() << std::endl;

        for (auto v : result.isomorphism)
            std::cout << "(" << v.first << " -> " << v.second << ") ";
        std::cout << std::endl;

        std::cout << overall_time.count();
        if (! result.times.empty()) {
            for (auto t : result.times)
                std::cout << " " << t.count();
        }
        std::cout << std::endl;

        for (auto & p : result.isomorphism) {
            if (graphs.first.vertex_labels.at(p.first) != graphs.second.vertex_labels.at(p.second)) {
                std::cerr << "Oops! not an isomorphism due to vertex labels" << std::endl;
                return EXIT_FAILURE;
            }
        }

        for (auto & p : result.isomorphism) {
            for (auto & q : result.isomorphism) {
                if (graphs.first.edges.at(p.first).at(q.first) != graphs.second.edges.at(p.second).at(q.second)) {
                    std::cerr << "Oops! not an isomorphism due to edge " << std::to_string(p.first) << "--"
                        << std::to_string(q.first) << " (" << graphs.first.edges[p.first][q.first] << ") vs "
                        << std::to_string(p.second) << "--" << std::to_string(q.second) << " ("
                        << graphs.second.edges[p.second][q.second] << ")" << std::endl;
                    return EXIT_FAILURE;
                }
            }
        }

        return EXIT_SUCCESS;
    }
    catch (const po::error & e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Try " << argv[0] << " --help" << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception & e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

