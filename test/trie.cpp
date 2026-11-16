#include "trie.h"
#include <array>
#include <stdexcept>

namespace
{
using Key = std::vector<int>;
constexpr unsigned int KeyCount = 64;

Key makeKey( unsigned int mask )
{
    Key key;
    for( int bit = 0; bit < 6; ++bit )
    {
        if( mask & ( 1u << bit ) ) key.push_back( bit );
    }
    return key;
}

void checkQueries( trie<Key>& lookup, const std::array<bool, KeyCount>& stored )
{
    for( unsigned int query = 0; query < KeyCount; ++query )
    {
        Key key = makeKey( query );
        bool subset = query == 0 && stored[0];
        bool prefix = subset;
        for( unsigned int mask = 1; mask < KeyCount; ++mask )
        {
            if( !stored[mask] ) continue;
            if( ( mask & query ) == mask ) subset = true;
            Key candidate = makeKey( mask );
            if( candidate.size() <= key.size() &&
                std::equal( candidate.begin(), candidate.end(), key.begin() ) )
            {
                prefix = true;
            }
        }
        if( lookup.find_subset( key ) != subset || lookup.find_prefix( key ) != prefix )
        {
            throw std::runtime_error( "Trie subset/prefix mismatch" );
        }
    }
}
}

int main()
{
    trie<Key> lookup;
    std::array<bool, KeyCount> stored{};
    checkQueries( lookup, stored );
    for( unsigned int step = 0; step < KeyCount; ++step )
    {
        const unsigned int mask = ( step * 17 ) % KeyCount;
        Key key = makeKey( mask );
        if( !lookup.insert( key ) || !lookup.insert( key ) )
        {
            throw std::runtime_error( "Trie insertion failed" );
        }
        stored[mask] = true;
        checkQueries( lookup, stored );
    }
    for( unsigned int step = 0; step < KeyCount; ++step )
    {
        const unsigned int mask = ( step * 23 ) % KeyCount;
        Key key = makeKey( mask );
        lookup.erase( key );
        lookup.erase( key );
        stored[mask] = false;
        checkQueries( lookup, stored );
    }
}
