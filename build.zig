const std = @import("std");
const builtin = std.builtin;
const LinkMode = builtin.LinkMode;
const OptimizeMode = builtin.OptimizeMode;
const LtoMode = std.zig.LtoMode;
const Build = std.Build;

// ----- Hardcoded config -----

const lib_ver_str = "0.1.0";

const sources: []const []const u8 = &.{
    "src/utf.c",
    "src/termsize.c",
    "src/terminal.c",
    "src/screen.c",
    "src/escseq.c",
};
const tests: []const []const u8 = &.{
    "test/gradient.c",
    "test/label_gradient.c",
    "test/scralloc.c",
    "test/scrtxt.c",
    "test/seq.c",
    "test/splat_gradient.c",
    "test/termsize.c",
};

const base_flags: []const []const u8 = &.{
    "-std=c23",
    "-Wall",
    "-Wextra",
    "-pedantic",
    "-ffreestanding",
};
const release_flags: []const []const u8 = &.{
    "-Werror",
    "-DNDEBUG",
};
const debug_flags: []const []const u8 = &.{
    "-g3",
    "-DDEBUG",
};

// ----------------------------

pub fn build(b: *Build) !void {
    const run_test_step = b.step("run", "Run the given test");

    const test_source = b.option([]const u8, "test", "The test source file to build");
    const linkage = b.option(LinkMode, "link", "Build libescape as a static or dynamically linked library") orelse .static;
    const everything = b.option(bool, "everything", "Build everything for testing: libraries + tests") orelse false;
    const emit_asm = b.option(bool, "asm", "Emits assembly output of the current build for both tests and libraries.") orelse false;

    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    if (everything) {
        inline for (comptime std.enums.values(OptimizeMode)) |o| {
            inline for (comptime std.enums.values(LinkMode)) |l| {
                try installStep(b, emit_asm, tests, null, target, o, l);
            }
        }
    } else {
        try installStep(b, emit_asm, if (test_source) |source| &.{source} else null, run_test_step, target, optimize, linkage);
    }
}

const lib_ver = std.SemanticVersion.parse(lib_ver_str) catch @compileError("'" ++ lib_ver_str ++ "' is not valid semantic versioning");

fn installStep(
    b: *Build,
    emit_asm: bool,
    build_tests: ?[]const []const u8,
    run_test_step: ?*Build.Step,
    target: Build.ResolvedTarget,
    optimize: OptimizeMode,
    lib_linkage: LinkMode,
) !void {
    const is_debug = (optimize == .Debug);
    const cflags: []const []const u8 = if (is_debug) (base_flags ++ debug_flags) else (base_flags ++ release_flags);
    const cmod_opts: Build.Module.CreateOptions = .{
        .target = target,
        .optimize = optimize,
        .sanitize_c = if (is_debug) .full else .off,
        .strip = (optimize == .ReleaseFast),
        .link_libc = true,
    };

    const lib_mod = b.addModule("escape", cmod_opts);
    lib_mod.addCSourceFiles(.{
        .files = sources,
        .flags = cflags,
    });
    const libescape = b.addLibrary(.{
        .name = if (optimize == .Debug) "escape_g" else "escape",
        .linkage = lib_linkage,
        .root_module = lib_mod,
        .version = lib_ver,
    });
    libescape.lto = if (lib_linkage == .dynamic and !is_debug) .full else .none;
    if (emit_asm) {
        try emitAssembly(b, libescape);
    }

    if (build_tests) |test_paths| for (test_paths) |test_path| {
        const test_exe_mod = b.addModule(test_path, cmod_opts);
        test_exe_mod.linkLibrary(libescape);
        test_exe_mod.addCSourceFile(.{
            .file = b.path(test_path),
            .flags = cflags,
        });
        const test_exe = b.addExecutable(.{
            .name = b.fmt("test-{s}-{c}_{s}", .{
                test_path["test/".len .. test_path.len - 2], // remove dir and file extension
                switch (lib_linkage) {
                    .dynamic => @as(u8, 'd'),
                    .static => @as(u8, 's'),
                },
                switch (optimize) {
                    .Debug => "g",
                    .ReleaseFast => "rf",
                    .ReleaseSafe => "rs",
                    .ReleaseSmall => "rz",
                },
            }),
            .linkage = .dynamic,
            .root_module = test_exe_mod,
        });
        test_exe.lto = if (is_debug) .none else .full;
        if (emit_asm) {
            try emitAssembly(b, test_exe);
        }
        if (run_test_step) |run_step| {
            const run_cmd = b.addRunArtifact(test_exe);
            if (b.args) |args| {
                run_cmd.addArgs(args);
            }
            run_step.dependOn(&run_cmd.step);
            run_step.dependOn(b.getInstallStep()); // always install on run step
        }
        b.installArtifact(test_exe);
    };
    b.installArtifact(libescape);
}

fn emitAssembly(b: *Build, artifact: *Build.Step.Compile) !void {
    const install_asm = b.addInstallBinFile(artifact.getEmittedAsm(), b.fmt("{s}.s", .{artifact.name}));
    b.getInstallStep().dependOn(&install_asm.step);
}
