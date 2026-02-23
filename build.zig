const std = @import("std");
const builtin = std.builtin;

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

fn getTestExeName(b: *std.Build, src_name: []const u8, linkage: builtin.LinkMode, optimize: builtin.OptimizeMode) ![]const u8 {
    const prefix: []const u8 = "test-";
    const suffix: []const u8 = switch (optimize) {
        .Debug => "-g",
        .ReleaseFast => "-rf",
        .ReleaseSafe => "-rs",
        .ReleaseSmall => "-rz",
    };

    var exe_name: std.ArrayList(u8) = .empty;

    try exe_name.appendSlice(b.allocator, prefix);
    try exe_name.appendSlice(b.allocator, src_name);
    try exe_name.appendSlice(b.allocator, switch (linkage) {
        .dynamic => "_d",
        .static => "_s",
    });
    try exe_name.appendSlice(b.allocator, suffix);

    return try exe_name.toOwnedSlice(b.allocator);
}

fn getTestSourcePath(b: *std.Build, test_name: []const u8) !std.Build.LazyPath {
    var src_path: std.ArrayList(u8) = .empty;

    try src_path.appendSlice(b.allocator, test_dir);
    try src_path.appendSlice(b.allocator, test_name);
    try src_path.appendSlice(b.allocator, ".c");

    return b.path(try src_path.toOwnedSlice(b.allocator));
}

pub fn build(b: *std.Build) !void {
    const run_test_step = b.step("run", "Run the given test");
    const build_test = b.option([]const u8, "test", "The test source file to build");
    const linkage = b.option(builtin.LinkMode, "link", "Build libescape as a static or dynamically linked library.") orelse .static;
    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{ .whitelist = supported_targets });

    const is_debug = (optimize == .Debug);

    const libmod = b.addModule("escape", .{
        .target = target,
        .optimize = optimize,
        .sanitize_c = if (is_debug) .full else .off,
        .strip = !is_debug,
        .link_libc = true,
    });

    const cflags = concat_flags: {
        const base = &[_][]const u8{
            "-std=c23",
            "-Wall",
            "-Wextra",
        };
        const release = &[_][]const u8{
            "-Werror",
        };
        const debug = &[_][]const u8{
            "-g3",
        };
        break :concat_flags (if (is_debug) (base ++ debug) else (base ++ release));
    };

    libmod.addCMacro(if (is_debug) "DEBUG" else "DNDEBUG", "");
    libmod.addCSourceFiles(.{
        .files = sources,
        .flags = cflags,
    });

    const lib = b.addLibrary(.{
        .name = if (is_debug) "escape_g" else "escape",
        .linkage = linkage,
        .root_module = libmod,
        .version = lib_ver,
    });
    lib.lto = if (linkage == .dynamic and !is_debug) .full else .none;

    b.installArtifact(lib);

    if (build_test) |test_name| {
        // if (std.mem.eql(u8, test_name, "all")) {
        //     // compile all tests
        // }
        const mod = b.addModule(test_name, .{
            .target = target,
            .optimize = optimize,
            .sanitize_c = if (is_debug) .full else .off,
            .strip = !is_debug,
            .link_libc = true,
        });
        mod.linkLibrary(lib);
        mod.addCSourceFile(.{
            .file = try getTestSourcePath(b, test_name),
            .flags = cflags,
        });

        const test_exe = b.addExecutable(.{
            .name = try getTestExeName(b, test_name, linkage, optimize),
            .linkage = linkage,
            .root_module = mod,
        });
        test_exe.lto = if (linkage == .dynamic and !is_debug) .full else .none;

        const run_test_exe = b.addRunArtifact(test_exe);
        run_test_step.dependOn(&run_test_exe.step);

        b.installArtifact(test_exe);
    }
}
