#pragma once

/**
 * @file mono.hh
 * @brief Mono Runtime External Introspection Library
 *
 * A header-only C++ library for external runtime introspection of .NET assemblies
 * running on the Mono runtime. Commonly used for game modding and reverse engineering.
 *
 * @note This library requires external implementations of:
 *       - raw_read(uintptr_t address, void* buffer, size_t size): Read raw memory
 *       - read<T>(uintptr_t address): Read typed value from memory
 *       - mono_module: Base address of mono-2.0-bdwgc.dll
 *
 * @warning Memory offsets are version-specific. Tested with Unity Mono runtime.
 */

#include <Windows.h>
#include <codecvt>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <cstdio>

// ============================================================================
// Configuration Constants
// ============================================================================

namespace mono_offsets {

/**
 * @defgroup RootDomainOffsets Root Domain Structure Offsets
 * @brief Offsets within mono_root_domain_t structure
 * @{
 */
constexpr int ROOT_DOMAIN_ASSEMBLIES        = 0xC8;   ///< Pointer to assembly linked list
constexpr int ROOT_DOMAIN_ID                = 0xBC;   ///< Domain identifier
constexpr int ROOT_DOMAIN_JITTED_TABLE      = 0x148;  ///< JIT-compiled function table
/** @} */

/**
 * @defgroup ClassOffsets Class Structure Offsets
 * @brief Offsets within mono_class_t structure
 * @{
 */
constexpr int CLASS_NAME                    = 0x48;   ///< Class name string pointer
constexpr int CLASS_NAMESPACE               = 0x50;   ///< Namespace name string pointer
constexpr int CLASS_FIELDS                  = 0x98;   ///< Fields array pointer
constexpr int CLASS_METHODS                 = 0xA0;   ///< Methods array pointer
constexpr int CLASS_NUM_FIELDS              = 0x100;  ///< Number of fields
constexpr int CLASS_NUM_METHODS             = 0xFC;   ///< Number of methods (type 0,1)
constexpr int CLASS_NUM_METHODS_ALT         = 0xF0;   ///< Number of methods (type 4)
constexpr int CLASS_TYPE_FLAGS              = 0x2A;   ///< Type classification flags
constexpr int CLASS_RUNTIME_INFO            = 0xD0;   ///< Runtime info pointer
/** @} */

/**
 * @defgroup MethodOffsets Method Structure Offsets
 * @brief Offsets within mono_method_t structure
 * @{
 */
constexpr int METHOD_NAME                   = 0x18;   ///< Method name string pointer
/** @} */

/**
 * @defgroup FieldOffsets Field Structure Offsets
 * @brief Offsets within mono_class_field_t structure
 * @{
 */
constexpr int FIELD_NAME                    = 0x8;    ///< Field name string pointer
constexpr int FIELD_OFFSET                  = 0x18;   ///< Field memory offset
constexpr int FIELD_STRIDE                  = 0x20;   ///< Size of field entry in array
/** @} */

/**
 * @defgroup VTableOffsets VTable Structure Offsets
 * @brief Offsets within mono_vtable_t structure
 * @{
 */
constexpr int VTABLE_FLAGS                  = 0x30;   ///< VTable flags
constexpr int VTABLE_STATIC_DATA            = 0x40;   ///< Static field data offset
constexpr int VTABLE_KLASS                  = 0x0;    ///< Class pointer offset
constexpr int VTABLE_KLASS_SIZE             = 0x5C;   ///< Size field in class
constexpr int VTABLE_INITIALIZED_FLAG       = 0x4;    ///< Flag indicating static data initialized
/** @} */

/**
 * @defgroup ImageOffsets Image Structure Offsets
 * @brief Offsets within mono_image_t structure
 * @{
 */
constexpr int IMAGE_FLAGS                   = 0x1C;   ///< Image flags
constexpr int IMAGE_CLASS_CACHE             = 0x4C0;  ///< Class hash table offset
constexpr int IMAGE_TABLE_BASE              = 0xE;    ///< Table info base index
constexpr int IMAGE_DYNAMIC_FLAG            = 0x20;   ///< Dynamic image flag
/** @} */

/**
 * @defgroup AssemblyOffsets Assembly Structure Offsets
 * @brief Offsets within mono_assembly_t structure
 * @{
 */
constexpr int ASSEMBLY_IMAGE                = 0x60;   ///< MonoImage pointer
constexpr int ASSEMBLY_NAME                 = 0x10;   ///< Assembly name string
/** @} */

/**
 * @defgroup HashTableOffsets Hash Table Structure Offsets
 * @brief Offsets within mono_hash_table_t structure
 * @{
 */
constexpr int HASH_SIZE                     = 0x18;   ///< Hash table size
constexpr int HASH_DATA                     = 0x20;   ///< Hash table data array
constexpr int HASH_NEXT                     = 0x108;  ///< Next entry in chain
constexpr int HASH_KEY                      = 0x58;   ///< Key extraction offset
/** @} */

/**
 * @defgroup TableInfoOffsets Table Info Structure Offsets
 * @brief Offsets within mono_table_info_t structure
 * @{
 */
constexpr int TABLE_ROWS                    = 0x8;    ///< Row count offset
constexpr int TABLE_ROWS_MASK               = 0xFFFFFF; ///< Mask to extract row count
constexpr int TABLE_ENTRY_SIZE              = 0x10;   ///< Size of each table entry
constexpr int TABLE_MAX_ID                  = 55;     ///< Maximum table ID
/** @} */

/**
 * @defgroup JitTableOffsets JIT Table Structure Offsets
 * @brief Offsets within JIT function table structure
 * @{
 */
constexpr int JIT_TABLE_COUNT               = 0x8;    ///< Entry count in table
constexpr int JIT_TABLE_ENTRIES             = 0x10;   ///< Entries array start
constexpr int JIT_ENTRY_COUNT               = 0x4;    ///< Function count in entry
constexpr int JIT_ENTRY_FUNCTIONS           = 0x18;   ///< Functions array start
constexpr int JIT_FUNC_MONO_PTR             = 0x0;    ///< Mono method pointer
constexpr int JIT_FUNC_JIT_PTR              = 0x10;   ///< JIT-compiled code pointer
/** @} */

/**
 * @defgroup LinkedListOffsets Linked List Structure Offsets
 * @brief Offsets within glist_t structure
 * @{
 */
constexpr int GLIST_DATA                    = 0x0;    ///< Data pointer
constexpr int GLIST_NEXT                    = 0x8;    ///< Next node pointer
constexpr int POINTER_SIZE                  = 0x8;    ///< Size of pointer (64-bit)
/** @} */

/**
 * @defgroup TypeTokens Type Token Constants
 * @brief Mono metadata type token constants
 * @{
 */
constexpr unsigned int TYPE_DEF_TOKEN       = 0x02000000; ///< TypeDef token mask
constexpr int TYPE_DEF_TABLE_ID             = 2;          ///< TypeDef table index
/** @} */

/**
 * @defgroup BufferSizes Buffer Size Constants
 * @brief Buffer sizes for string operations
 * @{
 */
constexpr size_t NAME_BUFFER_SIZE           = 128;    ///< Buffer size for name strings
constexpr size_t UNICODE_ESCAPE_SIZE        = 32;     ///< Buffer size for unicode escape
/** @} */

/**
 * @defgroup SpecialCharacters Special Character Constants
 * @brief Special character detection values
 * @{
 */
constexpr std::uint8_t UNICODE_ESCAPE_MARKER = 0xEE;  ///< Marker for unicode escape sequences
/** @} */

} // namespace mono_offsets

// ============================================================================
// Utility Macros
// ============================================================================

/**
 * @brief Helper macro to define offset-based accessor methods
 * @param func Function name
 * @param type Return type
 * @param offset Memory offset from 'this' pointer
 */
#define OFFSET(func, type, offset) \
    type func { return read<type>(reinterpret_cast<uintptr_t>(this) + offset); }

// ============================================================================
// Global State
// ============================================================================

/**
 * @brief Maps Mono method metadata pointers to their JIT-compiled code addresses
 * @note Populated by mono::init_functions()
 */
inline std::unordered_map<uintptr_t, uintptr_t> functions;

// ============================================================================
// String Utility Functions
// ============================================================================

/**
 * @brief Converts a UTF-8 string to its UTF-16 representation
 * @param val UTF-8 encoded string
 * @return First UTF-16 code unit of the converted string
 * @warning Uses deprecated std::wstring_convert (C++17 deprecated)
 */
inline unsigned short utf8_to_utf16(const char* val) {
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::u16string dest = convert.from_bytes(val);
    if (dest.empty()) {
        return 0;
    }
    return static_cast<unsigned short>(dest[0]);
}

/**
 * @brief Reads a wide character string from memory
 * @param address Memory address to read from
 * @param size Number of bytes to read
 * @return String read from memory
 * @note Requires external raw_read() implementation
 */
inline std::string read_widechar(const std::uintptr_t address, const std::size_t size) {
    const auto buffer = std::make_unique<char[]>(size);
    raw_read(address, buffer.get(), size);
    return std::string(buffer.get());
}

/**
 * @brief Extracts a name string from a memory address, handling unicode escapes
 * @param name_ptr_address Address containing pointer to name string
 * @return Extracted name string, with unicode escapes formatted as \\uXXXX
 *
 * This helper eliminates code duplication across multiple name() methods.
 * Handles Cheat Engine-style unicode escape sequences.
 *
 * @see https://github.com/cheat-engine/cheat-engine/blob/8bdb7f28a9d40ccaa6f4116b13c570907ce9ee2e/Cheat%20Engine/MonoDataCollector/MonoDataCollector/PipeServer.cpp#L896
 */
inline std::string extract_mono_name(uintptr_t name_ptr_address) {
    auto name = read_widechar(read<uintptr_t>(name_ptr_address), mono_offsets::NAME_BUFFER_SIZE);

    if (!name.empty() && static_cast<std::uint8_t>(name[0]) == mono_offsets::UNICODE_ESCAPE_MARKER) {
        char name_buff[mono_offsets::UNICODE_ESCAPE_SIZE];
        std::snprintf(name_buff, mono_offsets::UNICODE_ESCAPE_SIZE, "\\u%04X",
                      utf8_to_utf16(name.c_str()));
        name = name_buff;
    }

    return name;
}

// ============================================================================
// Forward Declarations
// ============================================================================

struct mono_root_domain_t;
struct mono_class_t;
struct mono_vtable_t;

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Generic linked list node (GList)
 *
 * Used internally by Mono for assembly lists and other collections.
 */
struct glist_t {
    OFFSET(data, uintptr_t, mono_offsets::GLIST_DATA)
    OFFSET(next, uintptr_t, mono_offsets::GLIST_NEXT)
};

/**
 * @brief Root application domain
 *
 * The entry point for accessing Mono runtime data. Contains references
 * to all loaded assemblies and the JIT function table.
 */
struct mono_root_domain_t {
    OFFSET(get_domain_assemblies, glist_t*, mono_offsets::ROOT_DOMAIN_ASSEMBLIES)
    OFFSET(get_domain_id, int, mono_offsets::ROOT_DOMAIN_ID)
    OFFSET(get_jitted_function_table, uintptr_t, mono_offsets::ROOT_DOMAIN_JITTED_TABLE)
};

/**
 * @brief Metadata table information
 *
 * Contains information about metadata tables in an assembly image.
 */
struct mono_table_info_t {
    /**
     * @brief Gets the number of rows in this metadata table
     * @return Row count (masked to 24 bits)
     */
    int get_rows() {
        return read<int>(reinterpret_cast<uintptr_t>(this) + mono_offsets::TABLE_ROWS)
               & mono_offsets::TABLE_ROWS_MASK;
    }
};

/**
 * @brief Represents a .NET method
 *
 * Contains method metadata including name, signature, and code pointers.
 */
struct mono_method_t {
    /**
     * @brief Gets the method name
     * @return Method name string
     */
    std::string get_name() {
        return extract_mono_name(reinterpret_cast<uintptr_t>(this) + mono_offsets::METHOD_NAME);
    }
};

/**
 * @brief Represents a class field
 *
 * Contains field metadata including name, type, and memory offset.
 */
struct mono_class_field_t {
    OFFSET(get_offset, int, mono_offsets::FIELD_OFFSET)

    /**
     * @brief Gets the field name
     * @return Field name string
     */
    std::string get_name() {
        return extract_mono_name(reinterpret_cast<uintptr_t>(this) + mono_offsets::FIELD_NAME);
    }
};

/**
 * @brief Runtime information for a class
 *
 * Contains domain-specific runtime data for a class.
 */
struct mono_class_runtime_info_t {
    OFFSET(get_max_domain, int, 0x0)
};

/**
 * @brief Virtual method table for a class
 *
 * Contains the vtable and static field data for a class instance in a specific domain.
 */
struct mono_vtable_t {
    OFFSET(get_flags, byte, mono_offsets::VTABLE_FLAGS)

    /**
     * @brief Gets the address of static field data for this class
     * @return Address of static fields, or 0 if not initialized
     */
    uintptr_t get_static_field_data() {
        if ((this->get_flags() & mono_offsets::VTABLE_INITIALIZED_FLAG) == 0) {
            return 0;
        }

        const auto klass_ptr = read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + mono_offsets::VTABLE_KLASS);
        const auto size_value = read<int>(klass_ptr + mono_offsets::VTABLE_KLASS_SIZE);
        const auto data_offset = mono_offsets::VTABLE_STATIC_DATA + mono_offsets::POINTER_SIZE * size_value;

        return read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + data_offset);
    }
};

/**
 * @brief Represents a .NET class/type
 *
 * The primary interface for accessing class metadata, fields, and methods.
 */
struct mono_class_t {
    OFFSET(get_num_fields, int, mono_offsets::CLASS_NUM_FIELDS)
    OFFSET(get_runtime_info, mono_class_runtime_info_t*, mono_offsets::CLASS_RUNTIME_INFO)

    /**
     * @brief Gets the class name
     * @return Class name string
     */
    std::string get_name() {
        return extract_mono_name(reinterpret_cast<uintptr_t>(this) + mono_offsets::CLASS_NAME);
    }

    /**
     * @brief Gets the namespace name
     * @return Namespace string, empty if in global namespace
     */
    std::string get_namespace() {
        return extract_mono_name(reinterpret_cast<uintptr_t>(this) + mono_offsets::CLASS_NAMESPACE);
    }

    /**
     * @brief Gets the number of methods in this class
     * @return Method count based on class type classification
     *
     * The method count location varies based on the class type:
     * - Types 0,1: Standard classes - use CLASS_NUM_METHODS offset
     * - Type 4: Generic instances - use CLASS_NUM_METHODS_ALT offset
     * - Types 3,5: Arrays/pointers - always return 0
     */
    int get_num_methods() {
        constexpr int TYPE_MASK = 7;
        const auto type_flags = read<int>(reinterpret_cast<uintptr_t>(this) + mono_offsets::CLASS_TYPE_FLAGS);
        const auto class_type = (type_flags & TYPE_MASK) - 1;

        switch (class_type) {
            case 0:  // DEF - Standard class definition
            case 1:  // REF - Class reference
                return read<int>(reinterpret_cast<uintptr_t>(this) + mono_offsets::CLASS_NUM_METHODS);

            case 4:  // GINST - Generic instance
                return read<int>(reinterpret_cast<uintptr_t>(this) + mono_offsets::CLASS_NUM_METHODS_ALT);

            case 3:  // SZARRAY - Single-dimension array
            case 5:  // POINTER - Pointer type
            default:
                return 0;
        }
    }

    /**
     * @brief Gets a method by index
     * @param index Method index (0-based)
     * @return Pointer to method, or nullptr if invalid
     */
    mono_method_t* get_method(int index) {
        if (index < 0 || index >= get_num_methods()) {
            return nullptr;
        }

        const auto methods_ptr = read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + mono_offsets::CLASS_METHODS);
        return reinterpret_cast<mono_method_t*>(
            read<uintptr_t>(methods_ptr + mono_offsets::POINTER_SIZE * index)
        );
    }

    /**
     * @brief Gets a field by index
     * @param index Field index (0-based)
     * @return Pointer to field, or nullptr if invalid
     */
    mono_class_field_t* get_field(int index) {
        if (index < 0 || index >= get_num_fields()) {
            return nullptr;
        }

        const auto fields_ptr = read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + mono_offsets::CLASS_FIELDS);
        return reinterpret_cast<mono_class_field_t*>(
            fields_ptr + mono_offsets::FIELD_STRIDE * index
        );
    }

    /**
     * @brief Gets the vtable for this class in a specific domain
     * @param domain Target application domain
     * @return Pointer to vtable, or nullptr if not available
     */
    mono_vtable_t* get_vtable(mono_root_domain_t* domain) {
        if (!domain) {
            return nullptr;
        }

        const auto runtime_info = this->get_runtime_info();
        if (!runtime_info) {
            return nullptr;
        }

        const auto domain_id = domain->get_domain_id();
        if (runtime_info->get_max_domain() < domain_id) {
            return nullptr;
        }

        const auto vtable_offset = mono_offsets::POINTER_SIZE * domain_id + mono_offsets::POINTER_SIZE;
        return reinterpret_cast<mono_vtable_t*>(
            read<uintptr_t>(reinterpret_cast<uintptr_t>(runtime_info) + vtable_offset)
        );
    }

    /**
     * @brief Finds a method by name and returns its JIT-compiled code address
     * @param method_name Name of the method to find
     * @return Pointer to JIT-compiled method wrapper, or nullptr if not found
     * @note Requires mono::init_functions() to be called first
     */
    mono_method_t* find_method(const std::string& method_name) {
        uintptr_t mono_ptr = 0;

        const int num_methods = this->get_num_methods();
        for (int i = 0; i < num_methods; ++i) {
            const auto method = this->get_method(i);
            if (!method) {
                continue;
            }

            if (method->get_name() == method_name) {
                mono_ptr = reinterpret_cast<uintptr_t>(method);
                break;
            }
        }

        if (mono_ptr == 0) {
            return nullptr;
        }

        auto it = functions.find(mono_ptr);
        if (it == functions.end()) {
            return nullptr;
        }

        return reinterpret_cast<mono_method_t*>(it->second);
    }

    /**
     * @brief Finds a field by name
     * @param field_name Name of the field to find
     * @return Pointer to field, or nullptr if not found
     */
    mono_class_field_t* find_field(const std::string& field_name) {
        const int num_fields = this->get_num_fields();
        for (int i = 0; i < num_fields; ++i) {
            const auto field = this->get_field(i);
            if (!field) {
                continue;
            }

            if (field->get_name() == field_name) {
                return field;
            }
        }

        return nullptr;
    }
};

/**
 * @brief Hash table for class lookups
 *
 * Used internally by Mono images to cache class definitions.
 */
struct mono_hash_table_t {
    OFFSET(get_size, int, mono_offsets::HASH_SIZE)
    OFFSET(get_data, uintptr_t, mono_offsets::HASH_DATA)
    OFFSET(get_next_value, void*, mono_offsets::HASH_NEXT)
    OFFSET(get_key_extract, unsigned int, mono_offsets::HASH_KEY)

    /**
     * @brief Looks up an entry in the hash table by key
     * @tparam T Type to cast the result to
     * @param key Lookup key
     * @return Pointer to entry, or nullptr if not found
     */
    template<typename T>
    T* lookup(void* key) {
        const auto size = this->get_size();
        if (size == 0) {
            return nullptr;
        }

        const auto hash_index = reinterpret_cast<uintptr_t>(key) % static_cast<unsigned int>(size);
        const auto bucket_offset = get_data() + mono_offsets::POINTER_SIZE * hash_index;

        auto entry = static_cast<mono_hash_table_t*>(read<void*>(bucket_offset));
        if (!entry) {
            return nullptr;
        }

        constexpr int MAX_ITERATIONS = 10000; // Prevent infinite loops
        int iterations = 0;

        while (reinterpret_cast<void*>(entry->get_key_extract()) != key) {
            entry = static_cast<mono_hash_table_t*>(entry->get_next_value());
            if (!entry || ++iterations > MAX_ITERATIONS) {
                return nullptr;
            }
        }

        return reinterpret_cast<T*>(entry);
    }
};

/**
 * @brief Represents an assembly image (the in-memory representation of an assembly)
 *
 * Contains metadata tables and class cache for an assembly.
 */
struct mono_image_t {
    OFFSET(get_flags, int, mono_offsets::IMAGE_FLAGS)

    /**
     * @brief Gets metadata table information by table ID
     * @param table_id Metadata table identifier (0-55)
     * @return Pointer to table info, or nullptr if invalid table ID
     */
    mono_table_info_t* get_table_info(int table_id) {
        if (table_id < 0 || table_id > mono_offsets::TABLE_MAX_ID) {
            return nullptr;
        }

        const auto offset = mono_offsets::TABLE_ENTRY_SIZE * (table_id + mono_offsets::IMAGE_TABLE_BASE);
        return reinterpret_cast<mono_table_info_t*>(reinterpret_cast<uintptr_t>(this) + offset);
    }

    /**
     * @brief Gets a class by type token
     * @param type_id Type definition token (0x02XXXXXX format)
     * @return Pointer to class, or nullptr if not found
     */
    mono_class_t* get_class(int type_id) {
        // Check if this is a dynamic image
        if ((this->get_flags() & mono_offsets::IMAGE_DYNAMIC_FLAG) != 0) {
            return nullptr;
        }

        // Validate type token format (must be TypeDef token)
        if ((type_id & 0xFF000000) != mono_offsets::TYPE_DEF_TOKEN) {
            return nullptr;
        }

        auto* hash_table = reinterpret_cast<mono_hash_table_t*>(
            reinterpret_cast<uintptr_t>(this) + mono_offsets::IMAGE_CLASS_CACHE
        );

        return hash_table->lookup<mono_class_t>(reinterpret_cast<void*>(type_id));
    }
};

/**
 * @brief Represents a loaded assembly
 */
struct mono_assembly_t {
    OFFSET(get_image, mono_image_t*, mono_offsets::ASSEMBLY_IMAGE)
};

// ============================================================================
// Main API Functions
// ============================================================================

namespace mono {

/**
 * @brief Gets the root application domain
 * @return Pointer to root domain, or nullptr on failure
 * @note Requires mono_module to be defined as the base address of mono-2.0-bdwgc.dll
 */
inline mono_root_domain_t* get_root_domain() {
    constexpr uintptr_t ROOT_DOMAIN_OFFSET = 0x499c78;
    return reinterpret_cast<mono_root_domain_t*>(
        read<uintptr_t>(mono_module + ROOT_DOMAIN_OFFSET)
    );
}

/**
 * @brief Initializes the JIT function lookup table
 *
 * Scans the JIT function table and builds a map from Mono method metadata
 * pointers to their JIT-compiled code addresses.
 *
 * @note Must be called before using mono_class_t::find_method()
 * @credits niceone1 (https://www.unknowncheats.me/forum/3434741-post11.html)
 */
inline void init_functions() {
    const auto root_domain = get_root_domain();
    if (!root_domain) {
        return;
    }

    const auto jitted_table = root_domain->get_jitted_function_table();
    if (!jitted_table) {
        return;
    }

    const auto table_count = read<int>(jitted_table + mono_offsets::JIT_TABLE_COUNT);

    for (int i = 0; i < table_count; ++i) {
        const auto entry = read<uintptr_t>(
            jitted_table + mono_offsets::JIT_TABLE_ENTRIES + i * mono_offsets::POINTER_SIZE
        );
        if (!entry) {
            continue;
        }

        const auto func_count = read<int>(entry + mono_offsets::JIT_ENTRY_COUNT);
        for (int j = 0; j < func_count; ++j) {
            const auto function = read<uintptr_t>(
                entry + mono_offsets::JIT_ENTRY_FUNCTIONS + j * mono_offsets::POINTER_SIZE
            );
            if (!function) {
                continue;
            }

            const auto mono_ptr = read<uintptr_t>(function + mono_offsets::JIT_FUNC_MONO_PTR);
            const auto jitted_ptr = read<uintptr_t>(function + mono_offsets::JIT_FUNC_JIT_PTR);

            if (mono_ptr && jitted_ptr) {
                functions[mono_ptr] = jitted_ptr;
            }
        }
    }
}

/**
 * @brief Opens an assembly by name from a domain
 * @param domain Target application domain
 * @param name Assembly name to find
 * @return Pointer to assembly, or nullptr if not found
 */
inline mono_assembly_t* domain_assembly_open(mono_root_domain_t* domain, const std::string& name) {
    if (!domain) {
        return nullptr;
    }

    auto domain_assemblies = domain->get_domain_assemblies();
    if (!domain_assemblies) {
        return nullptr;
    }

    constexpr int MAX_ITERATIONS = 10000; // Prevent infinite loops
    int iterations = 0;

    while (domain_assemblies && iterations++ < MAX_ITERATIONS) {
        const auto data = domain_assemblies->data();
        if (data) {
            const auto data_name = read_widechar(
                read<uintptr_t>(data + mono_offsets::ASSEMBLY_NAME),
                mono_offsets::NAME_BUFFER_SIZE
            );

            if (data_name == name) {
                return reinterpret_cast<mono_assembly_t*>(data);
            }
        }

        const auto next = domain_assemblies->next();
        if (!next) {
            break;
        }
        domain_assemblies = reinterpret_cast<glist_t*>(next);
    }

    return nullptr;
}

/**
 * @brief Finds a class by assembly and fully-qualified class name
 * @param assembly_name Name of the assembly containing the class
 * @param class_name Fully-qualified class name (e.g., "Namespace.ClassName")
 * @return Pointer to class, or nullptr if not found
 */
inline mono_class_t* find_class(const std::string& assembly_name, const std::string& class_name) {
    const auto root_domain = get_root_domain();
    if (!root_domain) {
        return nullptr;
    }

    const auto domain_assembly = domain_assembly_open(root_domain, assembly_name);
    if (!domain_assembly) {
        return nullptr;
    }

    const auto mono_image = domain_assembly->get_image();
    if (!mono_image) {
        return nullptr;
    }

    const auto table_info = mono_image->get_table_info(mono_offsets::TYPE_DEF_TABLE_ID);
    if (!table_info) {
        return nullptr;
    }

    const auto row_count = table_info->get_rows();
    auto* hash_table = reinterpret_cast<mono_hash_table_t*>(
        reinterpret_cast<uintptr_t>(mono_image) + mono_offsets::IMAGE_CLASS_CACHE
    );

    for (int i = 0; i < row_count; ++i) {
        const auto type_token = mono_offsets::TYPE_DEF_TOKEN | (i + 1);
        const auto ptr = hash_table->lookup<mono_class_t>(reinterpret_cast<void*>(type_token));
        if (!ptr) {
            continue;
        }

        // Build fully qualified name
        std::string full_name = ptr->get_name();
        const std::string ns = ptr->get_namespace();
        if (!ns.empty()) {
            full_name = ns + "." + ptr->get_name();
        }

        if (full_name == class_name) {
            return ptr;
        }
    }

    return nullptr;
}

} // namespace mono
