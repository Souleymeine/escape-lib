const std = @import("std");
const path = std.fs.path;
const LinkMode = std.builtin.LinkMode;
const OptimizeMode = std.builtin.OptimizeMode;
const LtoMode = std.zig.LtoMode;
const Build = std.Build;

// ----- Hardcoded config -----

const lib_ver_str = "0.1.0";

const sources: []const []const u8 = &.{
    "src/core/term.c",
    "src/core/termsize.c",
    "src/core/escseq.c",
    "src/rndr/unicode.c",
    "src/rndr/screen.c",
};
const tests: []const []const u8 = &.{
    "test/gradient.c",
    "test/label_gradient.c",
    "test/scrtxt.c",
    "test/seq.c",
    "test/splat_gradient.c",
    "test/size.c",
};

const base_flags: []const []const u8 = &.{
    "-std=c23",
    "-Wall",
    "-Wextra",
    "-Wvla",
    "-pedantic-errors",
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
    const run_test_step = b.step("run", "Run the given source file linked against libescape");

    const exe_source = b.option([]const u8, "exesrc", "Build and link the given source file against libescape");
    const linkage = b.option(LinkMode, "link", "Build libescape") orelse .static;
    const everything = b.option(bool, "everything", "Build everything (libraries + tests)") orelse false;
    const emit_asm = b.option(bool, "asm", "Whether to emit assembly output of the current build for both 'exesrc's and libraries") orelse false;

    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    if (everything) {
        inline for (comptime std.enums.values(OptimizeMode)) |o| {
            inline for (comptime std.enums.values(LinkMode)) |l| {
                try installStep(b, emit_asm, tests, null, target, o, l);
            }
        }
    } else {
        try installStep(b, emit_asm, if (exe_source) |source| &.{source} else null, run_test_step, target, optimize, linkage);
    }
}

const lib_ver = std.SemanticVersion.parse(lib_ver_str) catch @compileError("'" ++ lib_ver_str ++ "' is not valid semantic versioning");

fn installStep(
    b: *Build,
    emit_asm: bool,
    build_exes: ?[]const []const u8,
    run_step: ?*Build.Step,
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

    if (build_exes) |src_paths| for (src_paths) |src_path| {
        const exe_mod = b.addModule(src_path, cmod_opts);
        exe_mod.linkLibrary(libescape);
        exe_mod.addCSourceFile(.{
            .file = b.path(src_path),
            .flags = cflags,
        });
        const exe = b.addExecutable(.{
            .name = b.fmt("{s}-{s}-{c}{s}", .{
                path.dirname(src_path) orelse @panic("Source files linked against libescape must not be in the project's root folder!"),
                path.stem(src_path),
                @as(u8, switch (lib_linkage) { // Cast is necessary since char literals are comptime_int
                    .dynamic => 'd',
                    .static => 's',
                }),
                switch (optimize) {
                    .Debug => "g",
                    .ReleaseFast => "rf",
                    .ReleaseSafe => "rs",
                    .ReleaseSmall => "rz",
                },
            }),
            .linkage = .dynamic,
            .root_module = exe_mod,
        });
        exe.lto = if (is_debug) .none else .full;
        if (emit_asm) {
            try emitAssembly(b, exe);
        }
        if (run_step) |step| {
            const run_cmd = b.addRunArtifact(exe);
            if (b.args) |args| {
                run_cmd.addArgs(args);
            }
            step.dependOn(&run_cmd.step);
            step.dependOn(b.getInstallStep()); // always install on run step
        }
        b.installArtifact(exe);
    };
    b.installArtifact(libescape);
}

// https://codeberg.org/ziglang/zig/issues/31415
fn emitAssembly(b: *Build, artifact: *Build.Step.Compile) !void {
    const install_asm = b.addInstallBinFile(artifact.getEmittedAsm(), b.fmt("{s}.s", .{artifact.name}));
    b.getInstallStep().dependOn(&install_asm.step);
}
