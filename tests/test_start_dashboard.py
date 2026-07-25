from __future__ import annotations

import shutil
import subprocess
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
START_SCRIPT = PROJECT_ROOT / "start_dashboard.bat"


def test_check_only_repairs_a_broken_virtual_environment(tmp_path: Path) -> None:
    project_dir = tmp_path / "project"
    project_dir.mkdir()
    shutil.copy2(START_SCRIPT, project_dir / START_SCRIPT.name)

    venv_dir = project_dir / ".venv"
    subprocess.run(
        ["py", "-3.12", "-m", "venv", str(venv_dir)],
        check=True,
        capture_output=True,
        text=True,
    )

    config_path = venv_dir / "pyvenv.cfg"
    config = config_path.read_text(encoding="utf-8")
    broken_config = []
    for line in config.splitlines():
        if line.startswith("home = "):
            line = r"home = Z:\missing-python-312"
        elif line.startswith("executable = "):
            line = r"executable = Z:\missing-python-312\python.exe"
        elif line.startswith("command = "):
            line = (
                rf"command = Z:\missing-python-312\python.exe -m venv {venv_dir}"
            )
        broken_config.append(line)
    config_path.write_text(
        "\n".join(broken_config) + "\n",
        encoding="utf-8",
    )

    venv_python = venv_dir / "Scripts" / "python.exe"
    broken = subprocess.run(
        [str(venv_python), "--version"],
        capture_output=True,
        text=True,
    )
    assert broken.returncode != 0

    result = subprocess.run(
        [
            "cmd.exe",
            "/d",
            "/c",
            "call start_dashboard.bat --check-only <nul",
        ],
        cwd=project_dir,
        capture_output=True,
        text=True,
    )

    assert result.returncode == 0, result.stdout + result.stderr
    assert "Repairing Python 3.12 virtual environment" in result.stdout
    assert "Environment check completed." in result.stdout
    subprocess.run(
        [str(venv_python), "--version"],
        check=True,
        capture_output=True,
        text=True,
    )
