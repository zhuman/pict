// ------------------------------------------------------------------------------
// Simplified trie; not thread-safe
// ------------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

//
//
//
template <class Item>
class trienode
{
public:
    typedef std::vector< std::pair<Item, trienode<Item>*> > TNodes;

    TNodes children;
    bool   valid;

    trienode() : valid( false ) {}

    typename TNodes::iterator lower_bound( const Item& item )
    {
        return std::lower_bound( children.begin(), children.end(), item,
            []( const typename TNodes::value_type& child, const Item& key )
            {
                return std::less<Item>()( child.first, key );
            } );
    }

    typename TNodes::iterator find( const Item& item )
    {
        auto found = lower_bound( item );
        return found != children.end() && !std::less<Item>()( item, found->first )
            ? found : children.end();
    }

    ~trienode()
    {
        for( typename TNodes::iterator ii = children.begin(); ii != children.end(); ++ii )
        {
            delete ii->second;
        }
    }
};

//
//
//
template <class Col>
class trie
{
    typedef typename trienode<typename Col::value_type>::TNodes TNodeCol;

public:
    //
    //
    //
    trie()
    {
        m_root = new trienode<typename Col::value_type>;
    }

    //
    //
    //
    ~trie()
    {
        delete m_root;
    }

    //
    //
    //
    trienode<typename Col::value_type>* get_root() { return m_root; }

    //
    // returns false if insert failed (OutOfMemory)
    //
    bool insert( Col &t )
    {
        trienode<typename Col::value_type>* current = m_root;
        for( typename Col::iterator ic = t.begin(); ic != t.end(); ++ic )
        {
            typename TNodeCol::iterator found;
            found = current->lower_bound( *ic );
            if( found == current->children.end() || std::less<typename Col::value_type>()( *ic, found->first ) )
            {
                std::unique_ptr<trienode<typename Col::value_type>> node;

                try
                {
                    node.reset( new trienode<typename Col::value_type> );
                }
                catch(const std::bad_alloc& )
                {
                    return( false );
                }

                found = current->children.insert( found, std::make_pair( *ic, node.get() ) );
                node.release();
            }
            current = found->second;
        }
        current->valid = true;

        return( true );
    }

    //
    // trienodes are not deallocated when an element is erased
    // this is for perf reasons; memory usage is not that important
    // the whole structure will be torn down in d'tor
    //
    void erase( Col& t )
    {
        trienode<typename Col::value_type>* node = pfind( t );
        if( nullptr == node ) return;
        node->valid = false;
    }

    //
    //
    //
    bool find_prefix( Col &t )
    {
        return( pfind_prefix( t ) != nullptr );
    }

    //
    // Returns true if the trie contains at least one stored key whose elements
    // form a subset of t. Both the stored keys and t are sorted with the same
    // ordering (operator< on Col::value_type), so a stored key is a subset of t
    // exactly when it is a subsequence of t. This is the polynomial-time
    // equivalent of asking "is any stored key a prefix of some permutation of t"
    // without enumerating t's permutations.
    //
    // Empty-key semantics deliberately match the prior find_prefix()+permutation
    // approach: an empty stored key is reported only when t itself is empty (the
    // old code never tested the root for a non-empty query), so behavior stays
    // bit-for-bit identical.
    //
    bool find_subset( Col &t )
    {
        if( t.begin() == t.end() ) return( m_root->valid );
        return psubset( m_root, t.begin(), t.end(), false );
    }

private:
    trienode<typename Col::value_type>* m_root;

    //
    // Recursive subset search. A stored key is found when we reach a valid node
    // by matching a subsequence of [qbegin, qend). checkValid is false only for
    // the initial (root) call with a non-empty query, so the empty key is not
    // treated as a match there - preserving the original find_prefix semantics.
    // Children are visited in ascending key order and the query is
    // sorted ascending, so once a child's label exceeds every remaining query
    // element we can stop.
    //
    bool psubset( trienode<typename Col::value_type>* node,
                  typename Col::iterator qbegin,
                  typename Col::iterator qend,
                  bool checkValid )
    {
        if( checkValid && node->valid ) return( true );

        // Wide nodes are cheaper to probe using the few labels in the query.
        if( node->children.size() > static_cast<size_t>( std::distance( qbegin, qend ) ) )
        {
            for( auto q = qbegin; q != qend; ++q )
            {
                auto child = node->find( *q );
                if( child != node->children.end() )
                {
                    auto next = q;
                    ++next;
                    if( psubset( child->second, next, qend, true ) ) return( true );
                }
            }
            return( false );
        }

        for( typename TNodeCol::iterator ci = node->children.begin();
             ci != node->children.end(); ++ci )
        {
            const typename Col::value_type& label = ci->first;

            // find label at or after qbegin (both query and children ascending)
            typename Col::iterator q = qbegin;
            while( q != qend && *q < label ) ++q;

            // label is larger than every remaining query element: no later
            // (even larger) child can match either
            if( q == qend ) break;

            // *q is equivalent to label -> descend, consuming this query element
            if( !( label < *q ) )
            {
                typename Col::iterator next = q;
                ++next;
                if( psubset( ci->second, next, qend, true ) ) return( true );
            }
        }

        return( false );
    }


    //
    //
    //
    trienode<typename Col::value_type>* pfind( Col &t )
    {
        trienode<typename Col::value_type> *current = m_root;
        for( typename Col::iterator ic = t.begin(); ic != t.end(); ++ic )
        {
            typename TNodeCol::iterator found = current->find( *ic );
            if( found == current->children.end() ) return( nullptr );

            current = found->second;
        }
        return( current->valid ? current : nullptr );
    }

    //
    //
    //
    trienode<typename Col::value_type>* pfind_prefix( Col &t )
    {
        trienode<typename Col::value_type> *current = m_root;
        for( typename Col::iterator ic = t.begin(); ic != t.end(); ++ic )
        {
            typename TNodeCol::iterator found = current->find( *ic );
            if( found == current->children.end() ) return( nullptr );

            current = found->second;

            if( current->valid ) return( current );
        }

        return( current->valid ? current : nullptr );
    }
};

