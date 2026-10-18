#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <limits>
#include <meta>
#include <minwindef.h>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <windows.h>

#include <psapi.h>
#include <shlwapi.h>

#include "core/utils/auto_writer.h"
#include "core/utils/log.h"

namespace gems
{

using IATOrdinalNames = std::unordered_map<::WORD, std::string_view>;

namespace impl
{

inline auto build_ordinal_iat_names(
    ::HMODULE export_owner,
    const std::unordered_map<std::string_view, void *> &iat_entries) -> IATOrdinalNames
{
    auto ordinal_names = IATOrdinalNames{};
    auto module_info = ::MODULEINFO{};
    if (!export_owner ||
        !::K32GetModuleInformation(::GetCurrentProcess(), export_owner, &module_info, sizeof(module_info)))
    {
        return {};
    }

    const auto *base = reinterpret_cast<const std::uint8_t *>(module_info.lpBaseOfDll);
    auto image_size = static_cast<std::size_t>(module_info.SizeOfImage);
    const auto in_image = [&](std::size_t offset, std::size_t size)
    { return offset <= image_size && size <= image_size - offset; };

    auto dos_header = ::IMAGE_DOS_HEADER{};
    if (!in_image(0, sizeof(dos_header)))
    {
        return {};
    }
    std::memcpy(&dos_header, base, sizeof(dos_header));
    if (dos_header.e_magic != IMAGE_DOS_SIGNATURE || dos_header.e_lfanew < 0)
    {
        return {};
    }

    const auto nt_offset = static_cast<std::size_t>(dos_header.e_lfanew);
    auto nt_headers = ::IMAGE_NT_HEADERS{};
    if (!in_image(nt_offset, sizeof(nt_headers)))
    {
        return {};
    }
    std::memcpy(&nt_headers, base + nt_offset, sizeof(nt_headers));
    if (nt_headers.Signature != IMAGE_NT_SIGNATURE ||
        nt_headers.FileHeader.SizeOfOptionalHeader < sizeof(nt_headers.OptionalHeader) ||
        !in_image(
            nt_offset + offsetof(::IMAGE_NT_HEADERS, OptionalHeader), nt_headers.FileHeader.SizeOfOptionalHeader) ||
        nt_headers.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC ||
        nt_headers.OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT ||
        nt_headers.OptionalHeader.SizeOfImage > image_size ||
        nt_headers.OptionalHeader.SizeOfImage < nt_offset + sizeof(nt_headers))
    {
        return {};
    }
    image_size = nt_headers.OptionalHeader.SizeOfImage;

    const auto &directory = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!directory.VirtualAddress || directory.Size < sizeof(::IMAGE_EXPORT_DIRECTORY) ||
        !in_image(directory.VirtualAddress, directory.Size))
    {
        return {};
    }

    auto exports = ::IMAGE_EXPORT_DIRECTORY{};
    std::memcpy(&exports, base + directory.VirtualAddress, sizeof(exports));
    const auto valid_array = [&](::DWORD rva, ::DWORD count, std::size_t element_size)
    { return rva != 0 && rva <= image_size && count <= (image_size - rva) / element_size; };
    if (!exports.NumberOfFunctions || !exports.NumberOfNames ||
        !valid_array(exports.AddressOfFunctions, exports.NumberOfFunctions, sizeof(::DWORD)) ||
        !valid_array(exports.AddressOfNames, exports.NumberOfNames, sizeof(::DWORD)) ||
        !valid_array(exports.AddressOfNameOrdinals, exports.NumberOfNames, sizeof(::WORD)))
    {
        return {};
    }

    for (std::size_t i = 0; i < exports.NumberOfNames; ++i)
    {
        auto name_rva = ::DWORD{};
        auto function_index = ::WORD{};
        std::memcpy(&name_rva, base + exports.AddressOfNames + i * sizeof(name_rva), sizeof(name_rva));
        std::memcpy(
            &function_index, base + exports.AddressOfNameOrdinals + i * sizeof(function_index), sizeof(function_index));
        if (!name_rva || !in_image(name_rva, 1) || function_index >= exports.NumberOfFunctions)
        {
            return {};
        }

        const auto *name = reinterpret_cast<const char *>(base + name_rva);
        const auto *name_end = static_cast<const char *>(std::memchr(name, '\0', image_size - name_rva));
        if (!name_end)
        {
            return {};
        }

        auto function_rva = ::DWORD{};
        std::memcpy(
            &function_rva,
            base + exports.AddressOfFunctions + static_cast<std::size_t>(function_index) * sizeof(function_rva),
            sizeof(function_rva));
        if (!function_rva || !in_image(function_rva, 1))
        {
            continue;
        }

        const auto ordinal = static_cast<std::uint64_t>(exports.Base) + function_index;
        if (ordinal > (std::numeric_limits<::WORD>::max)())
        {
            continue;
        }

        const auto entry = iat_entries.find(std::string_view{name, static_cast<std::size_t>(name_end - name)});
        if (entry != iat_entries.end())
        {
            ordinal_names.try_emplace(static_cast<::WORD>(ordinal), entry->first);
        }
    }

    return ordinal_names;
}

struct LoadedModule
{
    ::HMODULE handle;
    std::string name;
};

inline auto loaded_modules() -> std::vector<LoadedModule>
{
    auto loaded_modules = std::vector<LoadedModule>{};

    auto modules = std::array<::HMODULE, 1024zu>{};
    auto modules_span = std::span{modules};
    auto bytes_needed = ::DWORD{0};

    if (!::K32EnumProcessModules(
            ::GetCurrentProcess(), std::ranges::data(modules_span), modules_span.size_bytes(), &bytes_needed))
    {
        return loaded_modules;
    }

    const auto count = std::min<DWORD>(bytes_needed / sizeof(HMODULE), std::ranges::size(modules_span));

    for (const auto module : modules | std::views::take(count))
    {
        auto module_name = std::string(MAX_PATH, '\0');

        if (const auto length = ::GetModuleBaseNameA(
                ::GetCurrentProcess(), module, std::ranges::data(module_name), std::ranges::size(module_name));
            length > 0)
        {
            module_name.resize(length);
            loaded_modules.emplace_back(module, std::move(module_name));
        }
    }

    return loaded_modules;
}

inline auto patch_out_module(
    ::HMODULE module,
    std::string_view module_name,
    const std::unordered_map<std::string_view, void *> &iat_entries,
    const IATOrdinalNames &ordinal_names = {}) -> std::expected<void, std::string>
{
    static auto excluded = std::set<std::string>{"aclayers.dll"};

    auto name = std::string(MAX_PATH, '\0');

    if (const auto length =
            ::GetModuleBaseNameA(::GetCurrentProcess(), module, std::ranges::data(name), std::ranges::size(name));
        length > 0)
    {
        name.resize(length);

        const auto name_len = std::ranges::size(name);

        for (const auto &ex : excluded)
        {
            const auto ex_len = std::ranges::size(ex);
            log("comp: {} {} {} {} {}",
                name,
                name_len,
                ex,
                ex_len,
                ::StrCmpNICA(std::ranges::data(ex), std::ranges::data(name), name_len));

            if (ex_len == name_len)
            {
                if (::StrCmpNICA(std::ranges::data(ex), std::ranges::data(name), name_len) == 0)
                {
                    log("skipping: {}", name);
                    return {};
                }
            }
        }
    }

    auto *base = reinterpret_cast<std::uint8_t *>(module);

    auto *dos_header = reinterpret_cast<::IMAGE_DOS_HEADER *>(base);
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return std::unexpected("invalid dos header");
    }

    auto *nt_headers = reinterpret_cast<::IMAGE_NT_HEADERS *>(base + dos_header->e_lfanew);
    if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
    {
        return std::unexpected("invalid nt header");
    }

    const auto &import_directory = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

    if (import_directory.VirtualAddress == 0 || import_directory.Size == 0)
    {
        return {};
    }

    log("searching for imports...");

    auto *import_descriptor = reinterpret_cast<::IMAGE_IMPORT_DESCRIPTOR *>(base + import_directory.VirtualAddress);

    for (; import_descriptor->Name != 0; ++import_descriptor)
    {
        const auto *imported_dll = reinterpret_cast<const char *>(base + import_descriptor->Name);
        const auto length = std::strlen(imported_dll);

        if (length != std::ranges::size(module_name))
        {
            continue;
        }

        if (::StrCmpNICA(imported_dll, std::ranges::data(module_name), length) != 0)
        {
            continue;
        }

        for (const auto &[function_name, replacement] : iat_entries)
        {
            auto *thunk = reinterpret_cast<::IMAGE_THUNK_DATA32 *>(base + import_descriptor->FirstThunk);

            auto *original_thunk =
                import_descriptor->OriginalFirstThunk
                    ? reinterpret_cast<::IMAGE_THUNK_DATA32 *>(base + import_descriptor->OriginalFirstThunk)
                    : nullptr;

            if (!original_thunk)
            {
                continue;
            }

            for (; original_thunk->u1.AddressOfData != 0; ++original_thunk, ++thunk)
            {
                if (IMAGE_SNAP_BY_ORDINAL32(original_thunk->u1.Ordinal))
                {
                    const auto ordinal = static_cast<::WORD>(IMAGE_ORDINAL32(original_thunk->u1.Ordinal));
                    const auto entry = ordinal_names.find(ordinal);
                    if (entry == ordinal_names.end() || entry->second != function_name)
                    {
                        continue;
                    }
                }
                else
                {
                    auto *import_by_name =
                        reinterpret_cast<::IMAGE_IMPORT_BY_NAME *>(base + original_thunk->u1.AddressOfData);

                    const auto *imported_function = reinterpret_cast<const char *>(import_by_name->Name);
                    const auto func_length = std::strlen(imported_function);

                    if (func_length != std::ranges::size(function_name))
                    {
                        continue;
                    }

                    if (::StrCmpNICA(imported_function, std::ranges::data(function_name), func_length) != 0)
                    {
                        continue;
                    }
                }

                auto *slot = reinterpret_cast<void **>(&thunk->u1.Function);

                {
                    void *replacement_address = replacement;

                    const auto auto_writer = AutoWriter{slot, sizeof(replacement_address)};

                    log("patching {}::{} {} -> {}",
                        module_name,
                        function_name,
                        static_cast<void *>(slot),
                        replacement_address);

                    std::memcpy(slot, &replacement_address, sizeof(replacement_address));
                }
            }
        }
    }

    log("module patch done");

    return {};
}

}

inline auto load_system_dll(std::string_view module_name) -> ::HMODULE
{
    auto system_dir = std::string(MAX_PATH, '\0');
    const auto length = ::GetSystemDirectory(std::ranges::data(system_dir), std::ranges::size(system_dir));
    system_dir.resize(length);

    const auto full_path = std::format("{}\\{}", system_dir, module_name);
    log("loading: {}", full_path);

    return ::LoadLibrary(full_path.c_str());
}

inline auto iat_patcher(
    std::string_view module_name,
    const std::unordered_map<std::string_view, void *> &iat_entries,
    const IATOrdinalNames &ordinal_names = {})
{
    log("iat patcher for: {}", module_name);

    const auto modules = impl::loaded_modules();
    for (const auto &[module, name] : modules)
    {
        log("found module: {}", name);

        const auto res = impl::patch_out_module(module, module_name, iat_entries, ordinal_names);
        if (!res)
        {
            log("failed to patch {}: {}", name, res.error());
        }
    }
}
}
