#include "StringTableDumperIW4MS.h"

#include "Csv/CsvStream.h"

using namespace IW4MS;

namespace string_table
{
    void DumperIW4MS::DumpAsset(AssetDumpingContext& context, const XAssetInfo<AssetStringTable::Type>& asset)
    {
        const auto* stringTable = asset.Asset();
        const auto assetFile = context.OpenAssetFile(asset.m_name);

        if (!assetFile)
            return;

        CsvOutputStream csv(*assetFile);

        for (auto row = 0; row < stringTable->rowCount; row++)
        {
            for (auto column = 0; column < stringTable->columnCount; column++)
            {
                const auto* cell = &stringTable->values[column + row * stringTable->columnCount];
                if (cell->string != nullptr)
                {
                    csv.WriteColumn(cell->string);
                }
                else
                {
                    csv.WriteColumn("");
                }
            }

            csv.NextRow();
        }
    }
} // namespace string_table
