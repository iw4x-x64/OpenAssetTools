#pragma once

#include "Game/IW4MS/IW4MS.h"
#include "Menu/IMenuWriter.h"

#include <memory>
#include <string>

namespace menu
{
    class IWriterIW4MS : public IWriter
    {
    public:
        virtual void WriteFunctionDef(const std::string& functionName, const IW4MS::Statement_s* statement) = 0;
        virtual void WriteMenu(const IW4MS::menuDef_t& menu) = 0;
    };

    std::unique_ptr<IWriterIW4MS> CreateMenuWriterIW4MS(std::ostream& stream);
} // namespace menu
