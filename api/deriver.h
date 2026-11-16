#pragma once

#include "generator.h"
#include "trie.h"
#include <unordered_map>

namespace pictcore
{

typedef std::list<Exclusion*> ExclPtrList;
typedef std::vector<Exclusion::iterator> ExclTermIterList;

//
//
//
class ExclusionDeriver
{
public:
    ExclusionDeriver( Task* Task ) : m_task( Task ) {}

    void AddParameter( Parameter* param ) { m_parameters.push_back( param ); }
    std::pair<ExclusionCollection::iterator, bool> AddExclusion( Exclusion& exclusion, bool checkIfInCollection );

    ParamCollection&     GetParameters() { return( m_parameters ); }
    ExclusionCollection& GetExclusions() { return( m_exclusions ); }

    void DeriveExclusions();

private:
    struct CompletedState
    {
        size_t remainingBuckets;
        std::vector<ExclusionTerm> terms;
    };

    struct CompletedSearches
    {
        static constexpr size_t MaxStates = 65536;
        static constexpr size_t MaxTerms = 1048576;

        size_t exclusionCount = 0;
        size_t termCount = 0;
        std::unordered_multimap<size_t, CompletedState> states;
        std::deque<size_t> insertionOrder;
    };

    void buildExclusion( Exclusion &Excl, std::vector<ExclPtrList>::iterator begin,
                         ExclTermIterList& inserted, CompletedSearches& completed );
    bool consistent( Exclusion &a, Exclusion &b );
    bool alreadyInCollection( Exclusion &a );

    void markDeleted ( Exclusion *excl ) { excl->markDeleted(); m_deletedAtLeastOne = true; }
    void markObsolete( ExclusionCollection::iterator ie );
    void peformDelete();

    void printLookup() { printLookupNode( m_lookup.get_root(), 0 ); }
    void printLookupNode( trienode<Exclusion::_ExclusionVec::value_type>* node, int indent );

    ParamCollection                    m_parameters;
    ExclusionCollection                m_exclusions;
    Parameter*                         m_currentParam = nullptr;
    Task*                              m_task;
    std::vector<ExclPtrList>::iterator m_end;
    std::deque<Parameter* >            m_worklist;
    trie<Exclusion::_ExclusionVec>     m_lookup;

    bool m_deletedAtLeastOne = false;
};

}