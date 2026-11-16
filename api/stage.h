#pragma once
#include <cstddef>

#if defined(PICT_STAGE_TIMING)
#include <chrono>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <locale>
#include <mutex>
#include <sstream>
#include <thread>
#endif

namespace pictcore
{

class StageTimer
{
public:
#if defined(PICT_STAGE_TIMING)
    explicit StageTimer( const wchar_t* name, std::size_t item = 0 ) :
        m_name( name ), m_item( item ), m_exceptions( std::uncaught_exceptions() )
    {
        write( L"BEGIN" );
        m_started = std::chrono::steady_clock::now();
    }

    ~StageTimer() { Finish(); }

    void Finish()
    {
        if( m_finished ) return;
        m_elapsedMilliseconds = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - m_started ).count();
        m_finished = true;
        write( std::uncaught_exceptions() > m_exceptions ? L"ABORT" : L"END" );
    }
#else
    explicit StageTimer( const wchar_t*, std::size_t = 0 ) {}
    void Finish() {}
#endif

    StageTimer( const StageTimer& ) = delete;
    StageTimer& operator=( const StageTimer& ) = delete;

private:
#if defined(PICT_STAGE_TIMING)
    void write( const wchar_t* event )
    {
        const auto now = std::chrono::system_clock::now();
        const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>( now );
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>( now - seconds ).count();
        const std::time_t time = std::chrono::system_clock::to_time_t( seconds );
        std::tm utc = {};
#if defined(_MSC_VER)
        gmtime_s( &utc, &time );
#else
        gmtime_r( &time, &utc );
#endif
        std::wostringstream line;
        line.imbue( std::locale::classic() );
        line << L"[" << std::put_time( &utc, L"%Y-%m-%dT%H:%M:%S" )
             << L"." << std::setfill( L'0' ) << std::setw( 3 ) << milliseconds
             << L"Z] [stage] " << event << L" " << m_name
             << L" thread=" << std::this_thread::get_id();
        if( m_item ) line << L" item=" << m_item;
        if( m_finished )
        {
            line << L" elapsed_ms=" << std::fixed << std::setprecision( 3 )
                 << m_elapsedMilliseconds;
        }
        line << L"\n";
        const auto text = line.str();
        static std::mutex outputMutex;
        std::lock_guard<std::mutex> lock( outputMutex );
        std::wcerr.write( text.data(), static_cast<std::streamsize>( text.size() ) );
    }

    const wchar_t* m_name;
    std::size_t m_item;
    std::chrono::steady_clock::time_point m_started;
    int m_exceptions;
    double m_elapsedMilliseconds = 0;
    bool m_finished = false;
#endif
};

}
