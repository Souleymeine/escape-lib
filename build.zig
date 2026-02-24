const std = @import("std");
const builtin = std.builtin;
const LinkMode = builtin.LinkMode;
const OptimizeMode = builtin.OptimizeMode;
const LtoMode = std.zig.LtoMode;
const Build = std.Build;

// ----- Hardcoded config -----

const lib_ver_str = "0.1.0";

const test_dir = "test/";
const sources = &[_][]const u8{
    "src/utf.c",
    "src/termsize.c",
    "src/terminal.c",
    "src/screen.c",
    "src/escseq.c",
};
const tests = &[_][]const u8{
    "test/gradient.c",
    "test/label_gradient.c",
    "test/scralloc.c",
    "test/scrtxt.c",
    "test/seq.c",
    "test/splat_gradient.c",
};

const base_flags = &[_][]const u8{
    "-std=c23",
    "-Wall",
    "-Wextra",
    "-pedantic",
    "-ffreestanding",
};
const release_flags = &[_][]const u8{
    "-Werror",
};
const debug_flags = &[_][]const u8{
    "-g3",
};

const supported_targets = &[_]std.Target.Query{ // zig fmt: off
    .{ .os_tag = .windows, .cpu_arch = .x86 },
    .{ .os_tag = .windows, .cpu_arch = .x86_64 },
    .{ .os_tag = .linux,   .cpu_arch = .x86 },
    .{ .os_tag = .linux,   .cpu_arch = .x86_64 },
    .{ .os_tag = .linux,   .cpu_arch = .aarch64 },
    .{ .os_tag = .freebsd, .cpu_arch = .x86 },
    .{ .os_tag = .freebsd, .cpu_arch = .x86_64 },
    .{ .os_tag = .freebsd, .cpu_arch = .aarch64 },
    .{ .os_tag = .macos,   .cpu_arch = .aarch64 },
};
// zig fmt: on

// ----------------------------

const lib_ver = std.SemanticVersion.parse(lib_ver_str) catch |e| @compileLog(e);

fn getTestExeName(b: *Build, src_name: []const u8, linkage: LinkMode, optimize: OptimizeMode) ![]const u8 {
    var exe_name: std.ArrayList(u8) = try .initCapacity(b.allocator, 10 + src_name.len);

    try exe_name.appendSlice(b.allocator, "test-");
    try exe_name.appendSlice(b.allocator, src_name);
    try exe_name.appendSlice(b.allocator, switch (linkage) {
        .dynamic => "-d",
        .static => "-s",
    });
    try exe_name.appendSlice(b.allocator, switch (optimize) {
        .Debug => "_g",
        .ReleaseFast => "_rf",
        .ReleaseSafe => "_rs",
        .ReleaseSmall => "_rz",
    });

    return try exe_name.toOwnedSlice(b.allocator);
}

fn getTestSourcePath(b: *std.Build, test_name: []const u8) !std.Build.LazyPath {
    var src_path: std.ArrayList(u8) = try .initCapacity(b.allocator, test_dir.len + test_name.len + 2);

    try src_path.appendSlice(b.allocator, test_dir);
    try src_path.appendSlice(b.allocator, test_name);
    try src_path.appendSlice(b.allocator, ".c");

    return b.path(try src_path.toOwnedSlice(b.allocator));
}

fn createLibescape(b: *Build, cmod_opts: Build.Module.CreateOptions, cflags: []const []const u8, lto: LtoMode, optimize: OptimizeMode, linkage: LinkMode) *Build.Step.Compile {
    const lib_mod = b.addModule("escape", cmod_opts);
    lib_mod.addCMacro(if (optimize == .Debug) "DEBUG" else "DNDEBUG", "");
    lib_mod.addCSourceFiles(.{
        .files = sources,
        .flags = cflags,
    });
    const lib = b.addLibrary(.{
        .name = if (optimize == .Debug) "escape_g" else "escape",
        .linkage = linkage,
        .root_module = lib_mod,
        .version = lib_ver,
    });
    lib.lto = lto;
    return lib;
}

fn createTestExe(b: *Build, lib: *Build.Step.Compile, cmod_opts: Build.Module.CreateOptions, cflags: []const []const u8, lto: LtoMode, test_name: []const u8, linkage: LinkMode, optimize: OptimizeMode) !*Build.Step.Compile {
    const test_mod = b.addModule(test_name, cmod_opts);
    test_mod.linkLibrary(lib);
    test_mod.addCSourceFile(.{
        .file = try getTestSourcePath(b, test_name),
        .flags = cflags,
    });
    const test_exe = b.addExecutable(.{
        .name = try getTestExeName(b, test_name, linkage, optimize),
        .linkage = linkage,
        .root_module = test_mod,
    });
    test_exe.lto = lto;
    return test_exe;
}

fn getModProperties(t: Build.ResolvedTarget, o: OptimizeMode, l: LinkMode, lto: *LtoMode, cmod_opts: *Build.Module.CreateOptions, cflags: *[]const []const u8) void {
    const is_debug = (o == .Debug);
    lto.* = if (l == .dynamic and !is_debug) .full else .none;
    cmod_opts.* = .{
        .target = t,
        .optimize = o,
        .sanitize_c = if (is_debug) .full else .off,
        .strip = !is_debug,
        .link_libc = true,
    };
    cflags.* = (if (is_debug) (base_flags ++ debug_flags) else (base_flags ++ release_flags));
}

pub fn build(b: *Build) !void {
    const run_test_step = b.step("run", "Run the given test");

    const build_everything = b.option(bool, "everything", "Build everything for testing: libraries + tests + unit tests") orelse false;
    const build_test = b.option([]const u8, "test", "The test source file to build");
    const linkage = b.option(LinkMode, "link", "Build libescape as a static or dynamically linked library.") orelse .static;
    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{ .whitelist = supported_targets });

    var lto: LtoMode = undefined;
    var cmod_opts: Build.Module.CreateOptions = undefined;
    var cflags: []const []const u8 = undefined;

    // TODO : factor + fix broken control flow
    if (build_everything) {
        inline for ([_]OptimizeMode{ .Debug, .ReleaseSafe, .ReleaseFast, .ReleaseSmall }) |o| {
            inline for ([_]LinkMode{ .dynamic, .static }) |l| {
                getModProperties(target, o, l, &lto, &cmod_opts, &cflags);
                const lib = createLibescape(b, cmod_opts, cflags, lto, o, l);
                inline for (tests) |test_path| {
                    const test_exe = try createTestExe(b, lib, cmod_opts, cflags, lto, test_path[test_dir.len .. test_path.len - 2], l, o);
                    b.installArtifact(test_exe);
                }
                b.installArtifact(lib);
            }
        }
    } else {
        getModProperties(target, optimize, linkage, &lto, &cmod_opts, &cflags);
        const lib = createLibescape(b, cmod_opts, cflags, lto, optimize, linkage);
        if (build_test) |test_name| {
            const test_exe = try createTestExe(b, lib, cmod_opts, cflags, lto, test_name, linkage, optimize);
            run_test_step.dependOn(&b.addRunArtifact(test_exe).step);
            run_test_step.dependOn(b.getInstallStep()); // always install on test run
            b.installArtifact(test_exe);
        }
        b.installArtifact(lib);
    }
}
