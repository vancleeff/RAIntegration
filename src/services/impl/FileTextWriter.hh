#ifndef RA_SERVICES_FILETEXTWRITER
#define RA_SERVICES_FILETEXTWRITER
#pragma once

#include "services\TextWriter.hh"

#include "RA_StringUtils.h"
#include "ra_utility.h"

#include <fstream>

namespace ra {
namespace services {
namespace impl {

class FileTextWriter : public ra::services::TextWriter
{
public:
    explicit FileTextWriter(const std::wstring& sFilename, std::ios_base::openmode nMode = std::ios::out) noexcept
        : m_oStream(sFilename, std::ios::binary | nMode)
    {
    }

    void Write(_In_ const std::string& sText) override
    {
        if (m_oStream.is_open())
            m_oStream.write(sText.c_str(), ra::to_signed(sText.length()));
    }

    void Write(_In_ const std::wstring& sText) override
    {
        if (m_oStream.is_open())
        {
            std::string sNarrowText = ra::Narrow(sText);
            m_oStream.write(sNarrowText.c_str(), ra::to_signed(sNarrowText.length()));
        }
    }

    std::ofstream& GetFStream() { return m_oStream; }

private:
    std::ofstream m_oStream;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_FILETEXTWRITER
