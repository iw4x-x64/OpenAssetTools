#include "IwiLoader.h"

#include "IwiWaveletDecoder.h"

#include "Image/IwiTypes.h"
#include "Utils/Logging/Log.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <format>
#include <istream>
#include <iterator>
#include <type_traits>
#include <vector>

using namespace image;

namespace
{
    [[nodiscard]] bool IsWaveletFormat8(const int8_t format)
    {
        const auto value = static_cast<iwi8::IwiFormat>(format);
        return value == iwi8::IwiFormat::IMG_FORMAT_WAVELET_RGBA || value == iwi8::IwiFormat::IMG_FORMAT_WAVELET_RGB
               || value == iwi8::IwiFormat::IMG_FORMAT_WAVELET_LUMINANCE_ALPHA
               || value == iwi8::IwiFormat::IMG_FORMAT_WAVELET_LUMINANCE || value == iwi8::IwiFormat::IMG_FORMAT_WAVELET_ALPHA;
    }

    /** \brief How many samples per pixel the stream carries, which is not always the output stride. */
    [[nodiscard]] unsigned WaveletChannelCount(const int8_t format)
    {
        switch (static_cast<iwi8::IwiFormat>(format))
        {
        case iwi8::IwiFormat::IMG_FORMAT_WAVELET_RGBA:
            return 4u;
        case iwi8::IwiFormat::IMG_FORMAT_WAVELET_RGB:
            return 3u;
        case iwi8::IwiFormat::IMG_FORMAT_WAVELET_LUMINANCE_ALPHA:
            return 2u;
        default:
            return 1u;
        }
    }

    [[nodiscard]] CommonIwiMetaData MetaFromFlags8(const uint32_t flags)
    {
        return CommonIwiMetaData{
            .m_no_picmip = (flags & iwi8::IwiFlags::IMG_FLAG_NOPICMIP) != 0,
            .m_streaming = (flags & iwi8::IwiFlags::IMG_FLAG_STREAMING) != 0,
            .m_clamp_u = (flags & iwi8::IwiFlags::IMG_FLAG_CLAMP_U) != 0,
            .m_clamp_v = (flags & iwi8::IwiFlags::IMG_FLAG_CLAMP_V) != 0,
            .m_dynamic = (flags & iwi8::IwiFlags::IMG_FLAG_DYNAMIC) != 0,
        };
    }

    /**
     * \brief Decodes a whole wavelet mip pyramid out of the rest of the stream.
     *
     * Unlike the other formats there are no per level sizes to seek by: the levels are one
     * bitstream, smallest first, each predicted from the one before it.
     *
     * \param payloadSize How many bytes of \p stream belong to this image. The stream cannot simply
     *        be read to its end: it is often an entry inside a larger container, where the end is
     *        somewhere else entirely.
     */
    [[nodiscard]] bool LoadWaveletMipChain(std::istream& stream, Texture& texture, const int8_t format, const size_t payloadSize)
    {
        std::vector<char> data(payloadSize);
        stream.read(data.data(), static_cast<std::streamsize>(payloadSize));
        if (static_cast<size_t>(stream.gcount()) != payloadSize)
        {
            con::error("Unexpected eof of wavelet iwi");
            return false;
        }

        const auto channelCount = WaveletChannelCount(format);
        const auto outputStride = static_cast<unsigned>(texture.GetFormat()->GetPitch(0u, 1u));

        image::wavelet::Decoder decoder(
            reinterpret_cast<const uint8_t*>(data.data()), data.size(), channelCount, outputStride);

        // Each level is predicted from the one below it, so the previous level has to survive until
        // the next one is decoded. The texture's own buffers cannot serve as that scratch: the
        // refinement pass writes back into the parent, which would corrupt an already stored mip.
        std::vector<uint8_t> previous;
        std::vector<uint8_t> level;

        for (auto mipLevel = texture.GetMipMapCount() - 1; mipLevel >= 0; mipLevel--)
        {
            const auto width = std::max(1u, texture.GetWidth() >> mipLevel);
            const auto height = std::max(1u, texture.GetHeight() >> mipLevel);

            level.assign(static_cast<size_t>(width) * height * outputStride, 0u);
            if (!decoder.DecodeLevel(previous.empty() ? nullptr : previous.data(), level.data(), width, height))
            {
                con::error("Iwi wavelet stream ended early at mip level {}", mipLevel);
                return false;
            }

            std::memcpy(texture.GetBufferForMipLevel(mipLevel), level.data(), level.size());
            previous = level;
        }

        return true;
    }

    const ImageFormat* GetFormat6(int8_t format)
    {
        switch (static_cast<iwi6::IwiFormat>(format))
        {
        case iwi6::IwiFormat::IMG_FORMAT_BITMAP_RGBA:
            return &format::B8_G8_R8_A8;
        case iwi6::IwiFormat::IMG_FORMAT_BITMAP_RGB:
            return &format::B8_G8_R8;
        case iwi6::IwiFormat::IMG_FORMAT_BITMAP_ALPHA:
            return &format::A8;
        case iwi6::IwiFormat::IMG_FORMAT_DXT1:
            return &format::BC1;
        case iwi6::IwiFormat::IMG_FORMAT_DXT3:
            return &format::BC2;
        case iwi6::IwiFormat::IMG_FORMAT_DXT5:
            return &format::BC3;
        case iwi6::IwiFormat::IMG_FORMAT_DXN:
            return &format::BC5;
        case iwi6::IwiFormat::IMG_FORMAT_BITMAP_LUMINANCE_ALPHA:
            return &format::R8_A8;
        case iwi6::IwiFormat::IMG_FORMAT_BITMAP_LUMINANCE:
            return &format::R8;
        case iwi6::IwiFormat::IMG_FORMAT_WAVELET_RGBA: // used
        case iwi6::IwiFormat::IMG_FORMAT_WAVELET_RGB:  // used
        case iwi6::IwiFormat::IMG_FORMAT_WAVELET_LUMINANCE_ALPHA:
        case iwi6::IwiFormat::IMG_FORMAT_WAVELET_LUMINANCE:
        case iwi6::IwiFormat::IMG_FORMAT_WAVELET_ALPHA:
            con::error("Unsupported IWI format: {}", format);
            break;
        default:
            con::error("Unknown IWI format: {}", format);
            break;
        }

        return nullptr;
    }

    std::optional<IwiLoaderResult> LoadIwi6(std::istream& stream)
    {
        iwi6::IwiHeader header{};

        stream.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (stream.gcount() != sizeof(header))
        {
            con::error("IWI header corrupted");
            return std::nullopt;
        }

        const auto* format = GetFormat6(header.format);
        if (format == nullptr)
            return std::nullopt;

        auto width = header.dimensions[0];
        auto height = header.dimensions[1];
        auto depth = header.dimensions[2];
        auto hasMipMaps = !(header.flags & iwi6::IwiFlags::IMG_FLAG_NOMIPMAPS);

        std::unique_ptr<Texture> texture;
        if (header.flags & iwi6::IwiFlags::IMG_FLAG_CUBEMAP)
            texture = std::make_unique<TextureCube>(format, width, height, hasMipMaps);
        else if (header.flags & iwi6::IwiFlags::IMG_FLAG_VOLMAP)
            texture = std::make_unique<Texture3D>(format, width, height, depth, hasMipMaps);
        else
            texture = std::make_unique<Texture2D>(format, width, height, hasMipMaps);

        texture->Allocate();

        auto currentFileSize = sizeof(iwi6::IwiHeader) + sizeof(IwiVersionHeader);
        const auto mipMapCount = hasMipMaps ? texture->GetMipMapCount() : 1;

        for (auto currentMipLevel = mipMapCount - 1; currentMipLevel >= 0; currentMipLevel--)
        {
            const auto sizeOfMipLevel = texture->GetSizeOfMipLevel(currentMipLevel) * texture->GetFaceCount();
            currentFileSize += sizeOfMipLevel;

            if (currentMipLevel < static_cast<int>(std::extent_v<decltype(iwi6::IwiHeader::fileSizeForPicmip)>)
                && currentFileSize != header.fileSizeForPicmip[currentMipLevel])
            {
                con::error("Iwi has invalid file size for picmip {}", currentMipLevel);
                return std::nullopt;
            }

            stream.read(reinterpret_cast<char*>(texture->GetBufferForMipLevel(currentMipLevel)), sizeOfMipLevel);
            if (stream.gcount() != sizeOfMipLevel)
            {
                con::error("Unexpected eof of iwi in mip level {}", currentMipLevel);
                return std::nullopt;
            }
        }

        CommonIwiMetaData meta{
            .m_no_picmip = (header.flags & iwi6::IwiFlags::IMG_FLAG_NOPICMIP) != 0,
            .m_streaming = (header.flags & iwi6::IwiFlags::IMG_FLAG_STREAMING) != 0,
            .m_clamp_u = (header.flags & iwi6::IwiFlags::IMG_FLAG_CLAMP_U) != 0,
            .m_clamp_v = (header.flags & iwi6::IwiFlags::IMG_FLAG_CLAMP_V) != 0,
            .m_dynamic = (header.flags & iwi6::IwiFlags::IMG_FLAG_DYNAMIC) != 0,
        };

        return IwiLoaderResult{
            .m_version = IwiVersion::IWI_6,
            .m_meta = meta,
            .m_texture = std::move(texture),
        };
    }

    const ImageFormat* GetFormat8(int8_t format)
    {
        switch (static_cast<iwi8::IwiFormat>(format))
        {
        case iwi8::IwiFormat::IMG_FORMAT_BITMAP_RGBA:
            return &format::B8_G8_R8_A8;
        case iwi8::IwiFormat::IMG_FORMAT_BITMAP_RGB:
            return &format::B8_G8_R8;
        case iwi8::IwiFormat::IMG_FORMAT_BITMAP_ALPHA:
            return &format::A8;
        case iwi8::IwiFormat::IMG_FORMAT_DXT1:
            return &format::BC1;
        case iwi8::IwiFormat::IMG_FORMAT_DXT3:
            return &format::BC2;
        case iwi8::IwiFormat::IMG_FORMAT_DXT5:
            return &format::BC3;
        case iwi8::IwiFormat::IMG_FORMAT_DXN:
            return &format::BC5;
        case iwi8::IwiFormat::IMG_FORMAT_BITMAP_LUMINANCE_ALPHA:
            return &format::R8_A8;
        case iwi8::IwiFormat::IMG_FORMAT_BITMAP_LUMINANCE:
            return &format::R8;
        // The wavelet formats decode to plain samples, so they report the format their pixels end
        // up in. See IwiWaveletDecoder.
        case iwi8::IwiFormat::IMG_FORMAT_WAVELET_RGBA:
            return &format::B8_G8_R8_A8;
        case iwi8::IwiFormat::IMG_FORMAT_WAVELET_RGB:
            return &format::B8_G8_R8_A8;
        case iwi8::IwiFormat::IMG_FORMAT_WAVELET_LUMINANCE_ALPHA:
            return &format::R8_A8;
        case iwi8::IwiFormat::IMG_FORMAT_WAVELET_LUMINANCE:
            return &format::R8;
        case iwi8::IwiFormat::IMG_FORMAT_WAVELET_ALPHA:
            return &format::A8;
        case iwi8::IwiFormat::IMG_FORMAT_DXT3A_AS_LUMINANCE:
        case iwi8::IwiFormat::IMG_FORMAT_DXT5A_AS_LUMINANCE:
        case iwi8::IwiFormat::IMG_FORMAT_DXT3A_AS_ALPHA:
        case iwi8::IwiFormat::IMG_FORMAT_DXT5A_AS_ALPHA:
        case iwi8::IwiFormat::IMG_FORMAT_DXT1_AS_LUMINANCE_ALPHA:
        case iwi8::IwiFormat::IMG_FORMAT_DXN_AS_LUMINANCE_ALPHA:
        case iwi8::IwiFormat::IMG_FORMAT_DXT1_AS_LUMINANCE:
        case iwi8::IwiFormat::IMG_FORMAT_DXT1_AS_ALPHA:
            con::error("Unsupported IWI format: {}", format);
            break;
        default:
            con::error("Unknown IWI format: {}", format);
            break;
        }

        return nullptr;
    }

    // Shared by versions 8 and 9, which differ only in the version byte itself.
    // `version` is carried through so the result reports the one the file gave.
    std::optional<IwiLoaderResult> LoadIwi8(std::istream& stream, const IwiVersion version)
    {
        iwi8::IwiHeader header{};

        stream.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (stream.gcount() != sizeof(header))
        {
            con::error("IWI header corrupted");
            return std::nullopt;
        }

        const auto* format = GetFormat8(header.format);
        if (format == nullptr)
            return std::nullopt;

        auto width = header.dimensions[0];
        auto height = header.dimensions[1];
        auto depth = header.dimensions[2];
        auto hasMipMaps = !(header.flags & iwi8::IwiFlags::IMG_FLAG_NOMIPMAPS);

        std::unique_ptr<Texture> texture;
        if ((header.flags & iwi8::IwiFlags::IMG_FLAG_MAPTYPE_MASK) == iwi8::IwiFlags::IMG_FLAG_MAPTYPE_CUBE)
        {
            texture = std::make_unique<TextureCube>(format, width, height, hasMipMaps);
        }
        else if ((header.flags & iwi8::IwiFlags::IMG_FLAG_MAPTYPE_MASK) == iwi8::IwiFlags::IMG_FLAG_MAPTYPE_3D)
        {
            texture = std::make_unique<Texture3D>(format, width, height, depth, hasMipMaps);
        }
        else if ((header.flags & iwi8::IwiFlags::IMG_FLAG_MAPTYPE_MASK) == iwi8::IwiFlags::IMG_FLAG_MAPTYPE_2D)
        {
            texture = std::make_unique<Texture2D>(format, width, height, hasMipMaps);
        }
        else if ((header.flags & iwi8::IwiFlags::IMG_FLAG_MAPTYPE_MASK) == iwi8::IwiFlags::IMG_FLAG_MAPTYPE_1D)
        {
            con::error("Iwi has unsupported map type 1D");
            return std::nullopt;
        }
        else
        {
            con::error("Iwi has unsupported map type");
            return std::nullopt;
        }

        texture->Allocate();

        if (IsWaveletFormat8(header.format))
        {
            // A wavelet image has no per level sizes, so fileSizeForPicmip[0] is not a running total
            // the way it is for the other formats: it is the size of the whole file, and everything
            // after the header is the one bitstream.
            constexpr auto headerSize = sizeof(iwi8::IwiHeader) + sizeof(IwiVersionHeader);
            if (header.fileSizeForPicmip[0] <= headerSize)
            {
                con::error("Iwi wavelet has an invalid file size of {}", header.fileSizeForPicmip[0]);
                return std::nullopt;
            }

            if (!LoadWaveletMipChain(stream, *texture, header.format, header.fileSizeForPicmip[0] - headerSize))
                return std::nullopt;

            return IwiLoaderResult{
                .m_version = version,
                .m_meta = MetaFromFlags8(header.flags),
                .m_texture = std::move(texture),
            };
        }

        auto currentFileSize = sizeof(iwi8::IwiHeader) + sizeof(IwiVersionHeader);
        const auto mipMapCount = hasMipMaps ? texture->GetMipMapCount() : 1;

        for (auto currentMipLevel = mipMapCount - 1; currentMipLevel >= 0; currentMipLevel--)
        {
            const auto sizeOfMipLevel = texture->GetSizeOfMipLevel(currentMipLevel) * texture->GetFaceCount();
            currentFileSize += sizeOfMipLevel;

            // Version 8 stores the size corresponding to each picmip level, so in this
            // case it makes sense to validate every entry in the table.
            //
            // Version 9 appears to use this table somewhat differently. For example, out
            // of 1,650 version 9 images from one MW3 map port, 1,616 stored the total size
            // in all four entries while only 44 contained the expected descending sequence.
            //
            // Note that this does not mean that any mip data is missing. In both cases the
            // complete mip chain still adds up to the file size. In other words, for version
            // 9 we can only rely on entry zero, which contains the total size and is also
            // the only entry checked by the game itself.
            const auto entryIsMeaningful =
                currentMipLevel < static_cast<int>(std::extent_v<decltype(iwi8::IwiHeader::fileSizeForPicmip)>)
                && (version == IwiVersion::IWI_8 || currentMipLevel == 0);

            if (entryIsMeaningful && currentFileSize != header.fileSizeForPicmip[currentMipLevel])
            {
                con::error("Iwi has invalid file size for picmip {}", currentMipLevel);
                return std::nullopt;
            }

            stream.read(reinterpret_cast<char*>(texture->GetBufferForMipLevel(currentMipLevel)), sizeOfMipLevel);
            if (stream.gcount() != sizeOfMipLevel)
            {
                con::error("Unexpected eof of iwi in mip level {}", currentMipLevel);
                return std::nullopt;
            }
        }

        return IwiLoaderResult{
            .m_version = version,
            .m_meta = MetaFromFlags8(header.flags),
            .m_texture = std::move(texture),
        };
    }

    const ImageFormat* GetFormat13(int8_t format)
    {
        switch (static_cast<iwi13::IwiFormat>(format))
        {
        case iwi13::IwiFormat::IMG_FORMAT_BITMAP_RGBA:
            return &format::B8_G8_R8_A8;
        case iwi13::IwiFormat::IMG_FORMAT_BITMAP_RGB:
            return &format::B8_G8_R8;
        case iwi13::IwiFormat::IMG_FORMAT_BITMAP_ALPHA:
            return &format::A8;
        case iwi13::IwiFormat::IMG_FORMAT_DXT1:
            return &format::BC1;
        case iwi13::IwiFormat::IMG_FORMAT_DXT3:
            return &format::BC2;
        case iwi13::IwiFormat::IMG_FORMAT_DXT5:
            return &format::BC3;
        case iwi13::IwiFormat::IMG_FORMAT_DXN:
            return &format::BC5;
        case iwi13::IwiFormat::IMG_FORMAT_BITMAP_LUMINANCE_ALPHA:
            return &format::R8_A8;
        case iwi13::IwiFormat::IMG_FORMAT_BITMAP_LUMINANCE:
            return &format::R8;
        case iwi13::IwiFormat::IMG_FORMAT_WAVELET_RGBA: // used
        case iwi13::IwiFormat::IMG_FORMAT_WAVELET_RGB:  // used
        case iwi13::IwiFormat::IMG_FORMAT_WAVELET_LUMINANCE_ALPHA:
        case iwi13::IwiFormat::IMG_FORMAT_WAVELET_LUMINANCE:
        case iwi13::IwiFormat::IMG_FORMAT_WAVELET_ALPHA:
        case iwi13::IwiFormat::IMG_FORMAT_BITMAP_RGB565:
        case iwi13::IwiFormat::IMG_FORMAT_BITMAP_RGB5A3:
        case iwi13::IwiFormat::IMG_FORMAT_BITMAP_C8:
        case iwi13::IwiFormat::IMG_FORMAT_BITMAP_RGBA8:
        case iwi13::IwiFormat::IMG_FORMAT_A16B16G16R16F:
            con::error("Unsupported IWI format: {}", format);
            break;
        default:
            con::error("Unknown IWI format: {}", format);
            break;
        }

        return nullptr;
    }

    std::optional<IwiLoaderResult> LoadIwi13(std::istream& stream)
    {
        iwi13::IwiHeader header{};

        stream.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (stream.gcount() != sizeof(header))
        {
            con::error("IWI header corrupted");
            return std::nullopt;
        }

        const auto* format = GetFormat13(header.format);
        if (format == nullptr)
            return std::nullopt;

        auto width = header.dimensions[0];
        auto height = header.dimensions[1];
        auto depth = header.dimensions[2];
        auto hasMipMaps = !(header.flags & iwi13::IwiFlags::IMG_FLAG_NOMIPMAPS);

        std::unique_ptr<Texture> texture;
        if (header.flags & iwi13::IwiFlags::IMG_FLAG_CUBEMAP)
            texture = std::make_unique<TextureCube>(format, width, height, hasMipMaps);
        else if (header.flags & iwi13::IwiFlags::IMG_FLAG_VOLMAP)
            texture = std::make_unique<Texture3D>(format, width, height, depth, hasMipMaps);
        else
            texture = std::make_unique<Texture2D>(format, width, height, hasMipMaps);

        texture->Allocate();

        auto currentFileSize = sizeof(iwi13::IwiHeader) + sizeof(IwiVersionHeader);
        const auto mipMapCount = hasMipMaps ? texture->GetMipMapCount() : 1;

        for (auto currentMipLevel = mipMapCount - 1; currentMipLevel >= 0; currentMipLevel--)
        {
            const auto sizeOfMipLevel = texture->GetSizeOfMipLevel(currentMipLevel) * texture->GetFaceCount();
            currentFileSize += sizeOfMipLevel;

            if (currentMipLevel < static_cast<int>(std::extent_v<decltype(iwi13::IwiHeader::fileSizeForPicmip)>)
                && currentFileSize != header.fileSizeForPicmip[currentMipLevel])
            {
                con::error("Iwi has invalid file size for picmip {}", currentMipLevel);
                return std::nullopt;
            }

            stream.read(reinterpret_cast<char*>(texture->GetBufferForMipLevel(currentMipLevel)), sizeOfMipLevel);
            if (stream.gcount() != sizeOfMipLevel)
            {
                con::error("Unexpected eof of iwi in mip level {}", currentMipLevel);
                return std::nullopt;
            }
        }

        CommonIwiMetaData meta{
            .m_no_picmip = (header.flags & iwi13::IwiFlags::IMG_FLAG_NOPICMIP) != 0,
            .m_streaming = (header.flags & iwi13::IwiFlags::IMG_FLAG_STREAMING) != 0,
            .m_clamp_u = (header.flags & iwi13::IwiFlags::IMG_FLAG_CLAMP_U) != 0,
            .m_clamp_v = (header.flags & iwi13::IwiFlags::IMG_FLAG_CLAMP_V) != 0,
            .m_dynamic = (header.flags & iwi13::IwiFlags::IMG_FLAG_DYNAMIC) != 0,
            .m_gamma = header.gamma,
        };

        return IwiLoaderResult{
            .m_version = IwiVersion::IWI_13,
            .m_meta = meta,
            .m_texture = std::move(texture),
        };
    }

    const ImageFormat* GetFormat27(int8_t format)
    {
        switch (static_cast<iwi27::IwiFormat>(format))
        {
        case iwi27::IwiFormat::IMG_FORMAT_BITMAP_RGBA:
            return &format::R8_G8_B8_A8;
        case iwi27::IwiFormat::IMG_FORMAT_BITMAP_ALPHA:
            return &format::A8;
        case iwi27::IwiFormat::IMG_FORMAT_DXT1:
            return &format::BC1;
        case iwi27::IwiFormat::IMG_FORMAT_DXT3:
            return &format::BC2;
        case iwi27::IwiFormat::IMG_FORMAT_DXT5:
            return &format::BC3;
        case iwi27::IwiFormat::IMG_FORMAT_DXN:
            return &format::BC5;
        case iwi27::IwiFormat::IMG_FORMAT_A16B16G16R16F:
            assert(false); // Unsupported yet
            return &format::R16_G16_B16_A16_FLOAT;
        case iwi27::IwiFormat::IMG_FORMAT_BITMAP_RGB:
            return &format::B8_G8_R8; // This is a guess, idk the byte order as PC does not support this
        case iwi27::IwiFormat::IMG_FORMAT_BITMAP_LUMINANCE_ALPHA:
            return &format::R8_A8;
        case iwi27::IwiFormat::IMG_FORMAT_BITMAP_LUMINANCE:
            return &format::R8;
        case iwi27::IwiFormat::IMG_FORMAT_WAVELET_RGBA:
        case iwi27::IwiFormat::IMG_FORMAT_WAVELET_RGB:
        case iwi27::IwiFormat::IMG_FORMAT_WAVELET_LUMINANCE_ALPHA:
        case iwi27::IwiFormat::IMG_FORMAT_WAVELET_LUMINANCE:
        case iwi27::IwiFormat::IMG_FORMAT_WAVELET_ALPHA:
        case iwi27::IwiFormat::IMG_FORMAT_BITMAP_RGB565:
        case iwi27::IwiFormat::IMG_FORMAT_BITMAP_RGB5A3:
        case iwi27::IwiFormat::IMG_FORMAT_BITMAP_C8:
        case iwi27::IwiFormat::IMG_FORMAT_BITMAP_RGBA8:
            con::error("Unsupported IWI format: {}", format);
            break;
        default:
            con::error("Unknown IWI format: {}", format);
            break;
        }

        return nullptr;
    }

    std::optional<IwiLoaderResult> LoadIwi27(std::istream& stream)
    {
        iwi27::IwiHeader header{};

        stream.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (stream.gcount() != sizeof(header))
        {
            con::error("IWI header corrupted");
            return std::nullopt;
        }

        const auto* format = GetFormat27(header.format);
        if (format == nullptr)
            return std::nullopt;

        auto width = header.dimensions[0];
        auto height = header.dimensions[1];
        auto depth = header.dimensions[2];
        auto hasMipMaps = !(header.flags & iwi27::IwiFlags::IMG_FLAG_NOMIPMAPS);

        std::unique_ptr<Texture> texture;
        if (header.flags & iwi27::IwiFlags::IMG_FLAG_CUBEMAP)
            texture = std::make_unique<TextureCube>(format, width, height, hasMipMaps);
        else if (header.flags & iwi27::IwiFlags::IMG_FLAG_VOLMAP)
            texture = std::make_unique<Texture3D>(format, width, height, depth, hasMipMaps);
        else
            texture = std::make_unique<Texture2D>(format, width, height, hasMipMaps);

        texture->Allocate();

        auto currentFileSize = sizeof(iwi27::IwiHeader) + sizeof(IwiVersionHeader);
        const auto mipMapCount = hasMipMaps ? texture->GetMipMapCount() : 1;

        for (auto currentMipLevel = mipMapCount - 1; currentMipLevel >= 0; currentMipLevel--)
        {
            const auto sizeOfMipLevel = texture->GetSizeOfMipLevel(currentMipLevel) * texture->GetFaceCount();
            currentFileSize += sizeOfMipLevel;

            if (currentMipLevel < static_cast<int>(std::extent_v<decltype(iwi27::IwiHeader::fileSizeForPicmip)>)
                && currentFileSize != header.fileSizeForPicmip[currentMipLevel])
            {
                con::error("Iwi has invalid file size for picmip {}", currentMipLevel);
                return std::nullopt;
            }

            stream.read(reinterpret_cast<char*>(texture->GetBufferForMipLevel(currentMipLevel)), sizeOfMipLevel);
            if (stream.gcount() != sizeOfMipLevel)
            {
                con::error("Unexpected eof of iwi in mip level {}", currentMipLevel);
                return std::nullopt;
            }
        }

        CommonIwiMetaData meta{
            .m_no_picmip = (header.flags & iwi27::IwiFlags::IMG_FLAG_NOPICMIP) != 0,
            .m_streaming = (header.flags & iwi27::IwiFlags::IMG_FLAG_STREAMING) != 0,
            .m_clamp_u = (header.flags & iwi27::IwiFlags::IMG_FLAG_CLAMP_U) != 0,
            .m_clamp_v = (header.flags & iwi27::IwiFlags::IMG_FLAG_CLAMP_V) != 0,
            .m_dynamic = (header.flags & iwi27::IwiFlags::IMG_FLAG_DYNAMIC) != 0,
            .m_gamma = header.gamma,
        };

        return IwiLoaderResult{
            .m_version = IwiVersion::IWI_27,
            .m_meta = meta,
            .m_texture = std::move(texture),
        };
    }
} // namespace

namespace image
{
    std::optional<IwiLoaderResult> LoadIwi(std::istream& stream)
    {
        IwiVersionHeader iwiVersionHeader{};

        stream.read(reinterpret_cast<char*>(&iwiVersionHeader), sizeof(iwiVersionHeader));
        if (stream.gcount() != sizeof(iwiVersionHeader))
        {
            con::error("IWI version header corrupted");
            return std::nullopt;
        }

        if (iwiVersionHeader.tag[0] != 'I' || iwiVersionHeader.tag[1] != 'W' || iwiVersionHeader.tag[2] != 'i')
        {
            con::error("Invalid IWI magic");
            return std::nullopt;
        }

        switch (iwiVersionHeader.version)
        {
        case 6:
            return LoadIwi6(stream);

        case 8:
            return LoadIwi8(stream, IwiVersion::IWI_8);

        case 9:
            return LoadIwi8(stream, IwiVersion::IWI_9);

        case 13:
            return LoadIwi13(stream);

        case 27:
            return LoadIwi27(stream);

        default:
            break;
        }

        con::error("Unknown IWI version {}", iwiVersionHeader.version);
        return std::nullopt;
    }
} // namespace image
