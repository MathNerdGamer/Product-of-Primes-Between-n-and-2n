#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <numeric>
#include <sstream>
#include <thread>
#include <vector>

#include <NTL/ZZ.h>
#include <NTL/RR.h>
#include <omp.h>


// Counterexample:
// n = integer where counterexample occurs
// product = product of primes in [n, 2*n]
// exp = 2^n
struct Counterexample
{
    int64_t n;
    NTL::ZZ product;
    NTL::ZZ exp;
};

// Exponential:
// value = 2^exponent
struct Exponential
{
    NTL::ZZ value;
    int  exponent;
};

// List of all primes up to some finite limit (provided by file).
std::vector<std::int64_t> primes;

// Collects primes from file, using upper_bound to stop early if not all primes required.
void build_primes( std::string const &file_name, int upper_bound );

// Returns std::vector of primes in the interval [first, second].
std::vector<int64_t> primes_between( std::int64_t first,
                                     std::int64_t second );

int main(int argc, char **argv)
{
    using namespace std::chrono;
    auto start_time{ high_resolution_clock::now() };
    std::ofstream out_file{ "results.txt" };

    int upper_bound = 328;
    constexpr int MAX_BOUND = 328;
    // Need only check up to this limit for all possible counterexamples.
    // https://math.stackexchange.com/a/4497175/114928

    // Get upper bound.
    if( argc > 1 )
    {
        std::string arg{ argv[1] };
        
        try
        {
            upper_bound = std::min(std::stoi( arg ), MAX_BOUND);
        }
        catch(...)
        {
            // Stick with default value if something bad happens.
            upper_bound = 328;
        }
    }

    build_primes( "primes.txt", upper_bound );


    std::vector<Counterexample> counterexamples; // List of counterexamples.
    std::vector<Exponential> exponentials;       // List of exponentials.

    // Buffers to store thread-local vectors of counterexamples.
    std::vector<std::vector<Counterexample>> buffers;

    // Start of OpenMP block.
    #pragma omp parallel num_threads(std::thread::hardware_concurrency())
    {
        auto nthreads = omp_get_num_threads();
        auto id = omp_get_thread_num();
        
        // Set up buffers and exponentials vectors.
        #pragma omp single
        {
            buffers.resize( nthreads );
            exponentials.resize( nthreads );
            
            for( auto i = 0; i < nthreads; ++i )
            {
                exponentials[i] = { NTL::ZZ{ 2 }, 1 };
            }
        }
        
        constexpr auto bound{ 65 };
        // primes[65] holds 313, the largest prime needed to be considered.

        // The Proposition at
        // https://math.stackexchange.com/questions/4495973/the-product-of-primes-between-n-and-2n-compared-to-2n
        // shows that one only needs to check numbers of the form (p_{n} - 1)/2.
        // For any counterexample, there is one at least as big in that form.
        #pragma omp for schedule(dynamic)
        for( auto n{ 1 }; n < bound; ++n )
        {
            auto N{ ( primes[n] - 1 ) / 2 };

            if( n % 1000 == 0 )
            {
                auto current_time{ high_resolution_clock::now() };

                auto current_duration{ duration_cast<microseconds>( current_time - start_time ) / 1000.0 };

                std::cout << "Thread " << id << " has reached " << N << " (n = " << n << ") after "
                          << current_duration.count() / 1000.0 << " seconds.\n";

                out_file  << "Thread " << id << " has reached " << N << " (n = " << n << ") after "
                          << current_duration.count() / 1000.0 << " seconds.\n";
            }

            // Get primes between N and 2N.
            auto prime_list{ primes_between(N, 2 * N) };

            // Calculate 2^n
            exponentials[id].value <<= N - exponentials[id].exponent;
            exponentials[id].exponent = N;
            
            // Calculate product of primes from list.
            auto product{ std::accumulate( std::begin( prime_list ),
                                           std::end( prime_list ),
                                           NTL::ZZ{ 1 },
                                           []( NTL::ZZ a, std::int64_t b )
                                           { return a * b; } ) };
            
            // Collect counterexamples.
            if( product < exponentials[id].value )
            {
                buffers[id].push_back({ N, product, exponentials[id].value });
            }
        }

        #pragma omp single
        {
            // Put all of the counterexamples together into the main counterexamples vector.
            for( auto &buffer : buffers ) {
                move( std::begin(buffer), std::end(buffer), std::back_inserter( counterexamples ) );
            }
        }
    }
    // End of OpenMP block.

    std::cout << "\n\n\n";

    std::sort( std::begin( counterexamples ), std::end( counterexamples ),
               []( Counterexample const &a, Counterexample const &b )
               { return a.n < b.n; } );

    auto last = std::end( counterexamples ) - 1;

    // Output counterexamples.
    for( auto counterexample : counterexamples )
    {
        std::cout << "Counterexample at n = " << counterexample.n << "\n";
        std::cout << "Product of primes between n and 2n = " << counterexample.product << "\n";
        std::cout << "2^n = " << counterexample.exp;

        out_file  << "Counterexample at n = " << counterexample.n << "\n";
        out_file  << "Product of primes between n and 2n = " << counterexample.product << "\n";
        out_file  << "2^n = " << counterexample.exp;

        if( counterexample.n != last->n )
        {
            std::cout << "\n\n";
            out_file  << "\n\n";
        }
    }

    auto final_time{ high_resolution_clock::now() };
    auto final_duration{ duration_cast<microseconds>( final_time - start_time ) / 1000.0 };

    std::cout << "Final case checked after " << final_duration.count() / 1000.0 << " seconds.\n";
    out_file  << "Final case checked after " << final_duration.count() / 1000.0 << " seconds.\n";

    return EXIT_SUCCESS;
}

void build_primes( std::string const &file_name, int upper_bound )
{
    std::ifstream in_file{ file_name };
    upper_bound *= 2;
    bool bound_found = false;

    // Build vector of primes
    for( std::string line; std::getline( in_file, line ) && !bound_found; )
    {
        std::istringstream stream{ line };

        for( int64_t prime; stream >> prime; )
        {
            primes.push_back( prime );

            if( prime >= upper_bound )
            {
                bound_found = true;
                break;
            }
        }

        if( bound_found )
        {
            break;
        }
    }
}

std::vector<int64_t> primes_between( std::int64_t first,
                                     std::int64_t second )
{
    std::vector<int64_t> prime_list;

    auto it = std::upper_bound( std::begin( primes ), std::end( primes ), first,
                                []( std::int64_t const a, std::int64_t const b )
                                { return a <= b; } );

    while( *it <= second )
    {
        prime_list.push_back( *it );
        ++it;
    }

    return prime_list;
}
