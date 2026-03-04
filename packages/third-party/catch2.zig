const std = @import("std");

const Dependency = @import("Dependency.zig");
const Config = Dependency.Config;

/// Compiles catch2 from source as a static library.
/// https://github.com/allyourcodebase/catch2
pub fn build(b: *std.Build, config: Config) Dependency {
    const upstream = b.dependency("catch2", .{});
    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libcpp = true,
    });

    const include = upstream.path("src");
    const source_root = include.path(b, "catch2");
    const user_config = b.addConfigHeader(
        .{
            .style = .{ .cmake = source_root.path(b, "catch_user_config.hpp.in") },
            .include_path = "catch2/catch_user_config.hpp",
        },
        .{
            .CATCH_CONFIG_PREFIX_ALL = false,
            .CATCH_CONFIG_CONSOLE_WIDTH = 80,
            .CATCH_CONFIG_FAST_COMPILE = false,
            .CATCH_CONFIG_DISABLE = false,
            .CATCH_CONFIG_DEFAULT_REPORTER = "console",
            .CATCH_CONFIG_FALLBACK_STRINGIFIER = null,
        },
    );

    mod.addCSourceFiles(.{
        .root = source_root,
        .files = &sources,
        .flags = &.{"--std=c++23"},
    });

    mod.addConfigHeader(user_config);
    mod.addIncludePath(include);

    const lib = b.addLibrary(.{
        .name = "catch2",
        .root_module = mod,
    });

    lib.installHeadersDirectory(include, "", .{
        .include_extensions = &.{".hpp"},
    });
    lib.installConfigHeader(user_config);

    return .{
        .upstream = upstream,
        .artifact = lib,
    };
}

const sources = .{
    "benchmark/catch_chronometer.cpp",
    "benchmark/detail/catch_analyse.cpp",
    "benchmark/detail/catch_benchmark_function.cpp",
    "benchmark/detail/catch_run_for_at_least.cpp",
    "benchmark/detail/catch_stats.cpp",

    "catch_approx.cpp",
    "catch_assertion_result.cpp",
    "catch_config.cpp",
    "catch_get_random_seed.cpp",
    "catch_message.cpp",
    "catch_registry_hub.cpp",
    "catch_session.cpp",
    "catch_tag_alias_autoregistrar.cpp",
    "catch_test_case_info.cpp",
    "catch_test_spec.cpp",
    "catch_timer.cpp",
    "catch_tostring.cpp",
    "catch_totals.cpp",
    "catch_translate_exception.cpp",
    "catch_version.cpp",
    "internal/catch_assertion_handler.cpp",
    "internal/catch_case_insensitive_comparisons.cpp",
    "internal/catch_clara.cpp",
    "internal/catch_commandline.cpp",
    "internal/catch_console_colour.cpp",
    "internal/catch_context.cpp",
    "internal/catch_debug_console.cpp",
    "internal/catch_debugger.cpp",
    "internal/catch_decomposer.cpp",
    "internal/catch_enforce.cpp",
    "internal/catch_enum_values_registry.cpp",
    "internal/catch_errno_guard.cpp",
    "internal/catch_exception_translator_registry.cpp",
    "internal/catch_fatal_condition_handler.cpp",
    "internal/catch_floating_point_helpers.cpp",
    "internal/catch_getenv.cpp",
    "internal/catch_istream.cpp",
    "internal/catch_jsonwriter.cpp",
    "internal/catch_lazy_expr.cpp",
    "internal/catch_leak_detector.cpp",
    "internal/catch_list.cpp",
    "internal/catch_message_info.cpp",
    "internal/catch_output_redirect.cpp",
    "internal/catch_parse_numbers.cpp",
    "internal/catch_polyfills.cpp",
    "internal/catch_random_number_generator.cpp",
    "internal/catch_random_seed_generation.cpp",
    "internal/catch_reporter_registry.cpp",
    "internal/catch_reporter_spec_parser.cpp",
    "internal/catch_reusable_string_stream.cpp",
    "internal/catch_run_context.cpp",
    "internal/catch_section.cpp",
    "internal/catch_singletons.cpp",
    "internal/catch_source_line_info.cpp",
    "internal/catch_startup_exception_registry.cpp",
    "internal/catch_stdstreams.cpp",
    "internal/catch_string_manip.cpp",
    "internal/catch_stringref.cpp",
    "internal/catch_tag_alias_registry.cpp",
    "internal/catch_test_case_info_hasher.cpp",
    "internal/catch_test_case_registry_impl.cpp",
    "internal/catch_test_case_tracker.cpp",
    "internal/catch_test_failure_exception.cpp",
    "internal/catch_test_registry.cpp",
    "internal/catch_test_spec_parser.cpp",
    "internal/catch_textflow.cpp",
    "internal/catch_uncaught_exceptions.cpp",
    "internal/catch_wildcard_pattern.cpp",
    "internal/catch_xmlwriter.cpp",

    "interfaces/catch_interfaces_capture.cpp",
    "interfaces/catch_interfaces_config.cpp",
    "interfaces/catch_interfaces_exception.cpp",
    "interfaces/catch_interfaces_generatortracker.cpp",
    "interfaces/catch_interfaces_registry_hub.cpp",
    "interfaces/catch_interfaces_reporter.cpp",
    "interfaces/catch_interfaces_reporter_factory.cpp",
    "interfaces/catch_interfaces_testcase.cpp",

    "generators/catch_generator_exception.cpp",
    "generators/catch_generators.cpp",
    "generators/catch_generators_random.cpp",

    "matchers/catch_matchers.cpp",
    "matchers/catch_matchers_container_properties.cpp",
    "matchers/catch_matchers_exception.cpp",
    "matchers/catch_matchers_floating_point.cpp",
    "matchers/catch_matchers_predicate.cpp",
    "matchers/catch_matchers_quantifiers.cpp",
    "matchers/catch_matchers_string.cpp",
    "matchers/catch_matchers_templated.cpp",
    "matchers/internal/catch_matchers_impl.cpp",

    "reporters/catch_reporter_automake.cpp",
    "reporters/catch_reporter_common_base.cpp",
    "reporters/catch_reporter_compact.cpp",
    "reporters/catch_reporter_console.cpp",
    "reporters/catch_reporter_cumulative_base.cpp",
    "reporters/catch_reporter_event_listener.cpp",
    "reporters/catch_reporter_helpers.cpp",
    "reporters/catch_reporter_json.cpp",
    "reporters/catch_reporter_junit.cpp",
    "reporters/catch_reporter_multi.cpp",
    "reporters/catch_reporter_registrars.cpp",
    "reporters/catch_reporter_sonarqube.cpp",
    "reporters/catch_reporter_streaming_base.cpp",
    "reporters/catch_reporter_tap.cpp",
    "reporters/catch_reporter_teamcity.cpp",
    "reporters/catch_reporter_xml.cpp",
};
