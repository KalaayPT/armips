#include "armips_ffi.h"

#include "Core/Assembler.h"
#include "Core/Common.h"
#include "Core/Misc.h"
#include "Util/FileSystem.h"

#include <cstring>
#include <string>
#include <vector>

// strdup may need this on some platforms
#ifdef _WIN32
#define _CRT_NONSTDC_NO_DEPRECATE
#include <string.h>
#else
#include <cstdlib>
#endif

// Helper to split "NAME=VALUE" into components
static bool parse_define(const char* def, std::string& name, std::string& value, bool& is_label) {
    const char* eq = strchr(def, '=');
    if (!eq) return false;

    name.assign(def, eq - def);
    value = eq + 1;

    // Check if value is numeric (label) or string (equation)
    // Simple check: if it parses as integer, it's a label
    char* endptr;
    strtoll(value.c_str(), &endptr, 0);
    is_label = (*endptr == '\0');

    return true;
}

extern "C" {

int armips_assemble(const ArmipsFFIArgs* args) {
    if (!args || !args->input_file) {
        return 1;
    }

    // Change working directory if specified
    fs::path old_path;
    if (args->working_dir) {
        old_path = fs::current_path();
        std::error_code ec;
        fs::current_path(args->working_dir, ec);
        if (ec) {
            // Can't really report this error through the FFI
            return 1;
        }
    }

    // Set up arguments
    ArmipsArguments settings;
    settings.inputFileName = args->input_file;
    settings.silent = args->silent != 0;
    settings.errorOnWarning = args->error_on_warning != 0;
    settings.showStats = args->show_stats != 0;

    if (args->temp_file) {
        settings.tempFileName = args->temp_file;
    }

    if (args->sym_file) {
        settings.symFileName = args->sym_file;
        settings.symFileVersion = args->sym_version;
    }

    // Collect errors
    std::vector<std::string> error_list;
    settings.errorsResult = &error_list;

    // Parse defines
    for (size_t i = 0; i < args->define_count; i++) {
        std::string name, value;
        bool is_label;

        if (!parse_define(args->defines[i], name, value, is_label)) {
            error_list.push_back(std::string("Invalid define: ") + args->defines[i]);
            continue;
        }

        Identifier id(name);

        if (is_label) {
            // It's a label definition
            int64_t val = strtoll(value.c_str(), nullptr, 0);
            settings.labels.emplace_back(LabelDefinition{id, val});
        } else {
            // It's an equation
            settings.equList.emplace_back(EquationDefinition{id, value});
        }
    }

    // Run assembler
    bool success = runArmips(settings);

    // Restore working directory
    if (!old_path.empty()) {
        fs::current_path(old_path);
    }

    // Copy errors to output
    if (!error_list.empty()) {
        const_cast<ArmipsFFIArgs*>(args)->error_count = error_list.size();
        const_cast<ArmipsFFIArgs*>(args)->errors =
            (char**)malloc(sizeof(char*) * error_list.size());

        for (size_t i = 0; i < error_list.size(); i++) {
            const_cast<ArmipsFFIArgs*>(args)->errors[i] = strdup(error_list[i].c_str());
        }
    }

    return success ? 0 : 1;
}

void armips_free_errors(char** errors, size_t count) {
    if (!errors) return;

    for (size_t i = 0; i < count; i++) {
        free(errors[i]);
    }
    free(errors);
}

void armips_version(int* major, int* minor, int* revision) {
    if (major) *major = ARMIPS_VERSION_MAJOR;
    if (minor) *minor = ARMIPS_VERSION_MINOR;
    if (revision) *revision = ARMIPS_VERSION_REVISION;
}

} // extern "C"
