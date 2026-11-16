#include "deriver.h"
#include <stdexcept>

using namespace pictcore;

namespace
{
void require( bool condition )
{
    if( !condition ) throw std::runtime_error( "Exclusion allocator regression" );
}

class CountingResource : public std::pmr::memory_resource
{
public:
    size_t allocations = 0;
    size_t outstanding = 0;

private:
    void* do_allocate( size_t bytes, size_t alignment ) override
    {
        void* result = std::pmr::new_delete_resource()->allocate( bytes, alignment );
        ++allocations;
        ++outstanding;
        return result;
    }

    void do_deallocate( void* pointer, size_t bytes, size_t alignment ) override
    {
        --outstanding;
        std::pmr::new_delete_resource()->deallocate( pointer, bytes, alignment );
    }

    bool do_is_equal( const std::pmr::memory_resource& other ) const noexcept override
    {
        return this == &other;
    }
};

void scratchCopiesUseResource()
{
    Parameter first( 2, 0, 2, L"first", false );
    Parameter second( 2, 1, 2, L"second", false );
    CountingResource resource;
    {
        Exclusion original( &resource );
        const size_t beforeInsert = resource.allocations;
        original.insert( ExclusionTerm( &second, 1 ) );
        require( resource.allocations >= beforeInsert + 2 );
        const size_t beforeCopy = resource.allocations;
        Exclusion copy( original, &resource );
        require( resource.allocations >= beforeCopy + 2 );
        require( copy.GetList().get_allocator().resource() == &resource );
        require( !copy.insert( ExclusionTerm( &second, 1 ) ).second );
        copy.insert( ExclusionTerm( &first, 0 ) );
        require( original.size() == 1 && copy.size() == 2 );
        require( copy.GetList().size() == copy.size() );
        require( copy.begin()->first == &first );
    }
    require( resource.outstanding == 0 );
}

void storedCopiesOutlivePool()
{
    Parameter parameter( 2, 0, 2, L"parameter", false );
    ExclusionCollection stored;
    CountingResource upstream;
    {
        std::pmr::unsynchronized_pool_resource pool( &upstream );
        Exclusion temporary( &pool );
        temporary.insert( ExclusionTerm( &parameter, 1 ) );
        temporary.markDeleted();
        require( stored.insert( temporary ).second );
        require( upstream.outstanding != 0 );
    }
    require( upstream.outstanding == 0 );
    const Exclusion& copy = *stored.begin();
    require( copy.size() == 1 && copy.isDeleted() );
    require( copy.begin()->first == &parameter && copy.begin()->second == 1 );
    Exclusion another( copy );
    require( another.GetList().get_allocator().resource() == std::pmr::get_default_resource() );
    require( another.GetList().size() == 1 );
    require( another.GetList()[0] == *copy.begin() );
}

void derivedExclusionsOutliveScratch()
{
    Task task( 1 );
    Parameter first( 2, 0, 2, L"first", false );
    Parameter second( 2, 1, 2, L"second", false );
    Parameter third( 2, 2, 2, L"third", false );
    ExclusionDeriver deriver( &task );
    deriver.AddParameter( &first );
    deriver.AddParameter( &second );
    deriver.AddParameter( &third );

    Exclusion left;
    left.insert( ExclusionTerm( &first, 0 ) );
    left.insert( ExclusionTerm( &second, 0 ) );
    Exclusion right;
    right.insert( ExclusionTerm( &first, 1 ) );
    right.insert( ExclusionTerm( &third, 0 ) );
    deriver.AddExclusion( left, true );
    deriver.AddExclusion( right, true );
    deriver.DeriveExclusions();

    Exclusion expected;
    expected.insert( ExclusionTerm( &second, 0 ) );
    expected.insert( ExclusionTerm( &third, 0 ) );
    require( deriver.GetExclusions().size() == 3 );
    auto found = deriver.GetExclusions().find( expected );
    require( found != deriver.GetExclusions().end() );
    Exclusion copy( *found );
    require( copy.GetList().size() == 2 );
    std::sort( copy.lbegin(), copy.lend() );
    std::sort( expected.lbegin(), expected.lend() );
    require( copy.GetList() == expected.GetList() );
}
}

int main()
{
    scratchCopiesUseResource();
    storedCopiesOutlivePool();
    derivedExclusionsOutliveScratch();
}
