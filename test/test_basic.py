from __future__ import annotations

import os
from pathlib import Path
import subprocess

import pytest


def streplace_bin() -> Path:
    repo_root = Path(__file__).resolve().parents[1]
    return Path(os.environ.get("STREPLACE_BIN", repo_root / "streplace"))


def run_streplace(args: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    bin_path = streplace_bin()
    if "STREPLACE_BIN" not in os.environ:
        subprocess.run(["make"], cwd=cwd, check=True, capture_output=True, text=True)
    if not bin_path.exists():
        subprocess.run(["make"], cwd=cwd, check=True, capture_output=True, text=True)
        if not bin_path.exists():
            raise FileNotFoundError(f"streplace binary not found after make: {bin_path}")
    return subprocess.run(
        [str(bin_path)] + args,
        cwd=cwd,
        check=True,
        capture_output=True,
        text=True,
    )


def test_simple_text_replacement(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("foo baz\n", encoding="utf-8")

    run_streplace(["foo=bar", str(target)], streplace.parent)
    assert target.read_text(encoding="utf-8") == "bar baz\n"


def test_regex_replacement_default_mode(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("fooo baz\n", encoding="utf-8")

    run_streplace(["fo+=bar", str(target)], streplace.parent)
    assert target.read_text(encoding="utf-8") == "bar baz\n"


def test_no_regex_literal_match(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("fo+o baz\n", encoding="utf-8")

    run_streplace(["-x", "fo+o=bar", str(target)], streplace.parent)
    assert target.read_text(encoding="utf-8") == "bar baz\n"


def test_ignore_case_matching(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("Foo baz\n", encoding="utf-8")

    run_streplace(["-i", "foo=bar", str(target)], streplace.parent)
    assert target.read_text(encoding="utf-8") == "bar baz\n"


def test_whole_words_only(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("foo foobar barfoo foo;\n", encoding="utf-8")

    run_streplace(["-w", "foo=bar", str(target)], streplace.parent)
    assert target.read_text(encoding="utf-8") == "bar foobar barfoo bar;\n"


def test_equals_separator(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("a=b x\n", encoding="utf-8")

    run_streplace(["--equals=::", "a=b::c=d", str(target)], streplace.parent)
    assert target.read_text(encoding="utf-8") == "c=d x\n"


def test_custom_dollar_reference(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("id=42\n", encoding="utf-8")

    run_streplace(
        ["--equals=::", "--dollar=SUB", "id=([0-9]+)::numSUB1", str(target)],
        streplace.parent,
    )
    assert target.read_text(encoding="utf-8") == "num42\n"


def test_whole_words_with_underscores(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("foo foo_bar foo2 barfoo foo; foo_ _foo\n", encoding="utf-8")

    run_streplace(["-w", "foo=bar", str(target)], streplace.parent)
    assert target.read_text(encoding="utf-8") == "bar foo_bar foo2 barfoo bar; foo_ _foo\n"


def test_custom_dollar_with_literal_dollar(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("id=42\n", encoding="utf-8")

    run_streplace(
        ["--equals=::", "--dollar=SUB", "id=([0-9]+)::$SUB1", str(target)],
        streplace.parent,
    )
    assert target.read_text(encoding="utf-8") == "$42\n"


def test_verbose_stats_output(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("foo\n", encoding="utf-8")

    result = run_streplace(["-v", "foo=bar", str(target)], streplace.parent)
    assert "1/1 file" in result.stdout


def test_dummy_mode_no_write(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("foo\n", encoding="utf-8")

    run_streplace(["-d", "foo=bar", str(target)], streplace.parent)
    assert target.read_text(encoding="utf-8") == "foo\n"


def test_dummy_mode_alias_zero(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("foo\n", encoding="utf-8")

    run_streplace(["-0", "foo=bar", str(target)], streplace.parent)
    assert target.read_text(encoding="utf-8") == "foo\n"


def test_preview_does_not_write(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("foo\n", encoding="utf-8")

    result = run_streplace(["-P", "foo=bar", str(target)], streplace.parent)
    assert "\33[01m" in result.stdout
    assert target.read_text(encoding="utf-8") == "foo\n"


def test_preview_context_zero(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("aaa\nfoo\nbbb\n", encoding="utf-8")

    result = run_streplace(["-P", "--context=0", "foo=bar", str(target)], streplace.parent)
    assert "aaa" not in result.stdout
    assert "bbb" not in result.stdout
    assert "--2--" in result.stdout


def test_preview_full_file_context(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("aaa\nfoo\nbbb\n", encoding="utf-8")

    result = run_streplace(["-P", "--context=-1", "foo=bar", str(target)], streplace.parent)
    assert "aaa" in result.stdout
    assert "bbb" in result.stdout


def test_recursive_processing(tmp_path: Path) -> None:
    streplace = streplace_bin()
    root = tmp_path / "root"
    subdir = root / "sub"
    subdir.mkdir(parents=True)
    target = subdir / "t.txt"
    target.write_text("foo\n", encoding="utf-8")

    run_streplace(["-r", "foo=bar", str(root)], streplace.parent)
    assert target.read_text(encoding="utf-8") == "bar\n"


def test_follow_links(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "t.txt"
    target.write_text("foo\n", encoding="utf-8")
    link = tmp_path / "link.txt"
    try:
        os.symlink(target, link)
    except OSError:
        pytest.skip("symlinks not supported on this platform")

    run_streplace(["-l", "foo=bar", str(link)], streplace.parent)
    assert target.read_text(encoding="utf-8") == "bar\n"


def test_only_extension_filter(tmp_path: Path) -> None:
    streplace = streplace_bin()
    txt_file = tmp_path / "t.txt"
    md_file = tmp_path / "t.md"
    txt_file.write_text("foo\n", encoding="utf-8")
    md_file.write_text("foo\n", encoding="utf-8")

    run_streplace(["-o", "txt", "foo=bar", str(txt_file), str(md_file)], streplace.parent)
    assert txt_file.read_text(encoding="utf-8") == "bar\n"
    assert md_file.read_text(encoding="utf-8") == "foo\n"


def test_html_only_extension_filter(tmp_path: Path) -> None:
    streplace = streplace_bin()
    html_file = tmp_path / "t.html"
    txt_file = tmp_path / "t.txt"
    html_file.write_text("foo\n", encoding="utf-8")
    txt_file.write_text("foo\n", encoding="utf-8")

    run_streplace(["-H", "foo=bar", str(html_file), str(txt_file)], streplace.parent)
    assert html_file.read_text(encoding="utf-8") == "bar\n"
    assert txt_file.read_text(encoding="utf-8") == "foo\n"


def test_c_only_extension_filter(tmp_path: Path) -> None:
    streplace = streplace_bin()
    c_file = tmp_path / "t.c"
    txt_file = tmp_path / "t.txt"
    c_file.write_text("foo\n", encoding="utf-8")
    txt_file.write_text("foo\n", encoding="utf-8")

    run_streplace(["-C", "foo=bar", str(c_file), str(txt_file)], streplace.parent)
    assert c_file.read_text(encoding="utf-8") == "bar\n"
    assert txt_file.read_text(encoding="utf-8") == "foo\n"


def test_ignore_errors_continues(tmp_path: Path) -> None:
    streplace = streplace_bin()
    root = tmp_path / "root"
    root.mkdir()
    ok_file = root / "ok.txt"
    bad_file = root / "bad.txt"
    ok_file.write_text("foo\n", encoding="utf-8")
    bad_file.write_text("foo\n", encoding="utf-8")
    bad_file.chmod(0)
    try:
        run_streplace(["-r", "-E", "foo=bar", str(root)], streplace.parent)
    finally:
        bad_file.chmod(0o600)

    assert ok_file.read_text(encoding="utf-8") == "bar\n"


def test_all_processes_dot_git(tmp_path: Path) -> None:
    streplace = streplace_bin()
    root = tmp_path / "root"
    git_dir = root / ".git"
    git_dir.mkdir(parents=True)
    git_file = git_dir / "config"
    git_file.write_text("foo\n", encoding="utf-8")

    run_streplace(["-r", "foo=bar", str(root)], streplace.parent)
    assert git_file.read_text(encoding="utf-8") == "foo\n"

    run_streplace(["-r", "--all", "foo=bar", str(root)], streplace.parent)
    assert git_file.read_text(encoding="utf-8") == "bar\n"


def test_rename_only_file(tmp_path: Path) -> None:
    streplace = streplace_bin()
    file_path = tmp_path / "foo.txt"
    file_path.write_text("foo\n", encoding="utf-8")

    run_streplace(["-N", "foo=bar", str(file_path)], streplace.parent)
    assert not file_path.exists()
    renamed = tmp_path / "bar.txt"
    assert renamed.read_text(encoding="utf-8") == "foo\n"


def test_rename_only_directory(tmp_path: Path) -> None:
    streplace = streplace_bin()
    dir_path = tmp_path / "foo_dir"
    dir_path.mkdir()
    (dir_path / "keep.txt").write_text("foo\n", encoding="utf-8")

    run_streplace(["-N", "foo=bar", str(dir_path)], streplace.parent)
    assert not dir_path.exists()
    renamed_dir = tmp_path / "bar_dir"
    assert (renamed_dir / "keep.txt").read_text(encoding="utf-8") == "foo\n"


def test_rename_and_modify_file(tmp_path: Path) -> None:
    streplace = streplace_bin()
    file_path = tmp_path / "foo.txt"
    file_path.write_text("foo\n", encoding="utf-8")

    run_streplace(["-A", "foo=bar", str(file_path)], streplace.parent)
    assert not file_path.exists()
    renamed = tmp_path / "bar.txt"
    assert renamed.read_text(encoding="utf-8") == "bar\n"


def test_modify_symlink_target(tmp_path: Path) -> None:
    streplace = streplace_bin()
    target = tmp_path / "foo.txt"
    target.write_text("foo\n", encoding="utf-8")
    link = tmp_path / "link.txt"
    try:
        os.symlink(target.name, link)
    except OSError:
        pytest.skip("symlinks not supported on this platform")

    run_streplace(["-s", "foo=bar", str(link)], streplace.parent)
    assert os.readlink(link) == "bar.txt"
    assert target.read_text(encoding="utf-8") == "foo\n"
