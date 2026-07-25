@echo off
setlocal

pushd "%~dp0"
if errorlevel 1 (
    echo [ERROR] Cannot open the ParallelPix project directory.
    pause
    exit /b 1
)

set "VENV_DIR=%CD%\.venv"
set "VENV_PYTHON=%VENV_DIR%\Scripts\python.exe"
set "CHECK_ONLY="

if /i "%~1"=="--check-only" set "CHECK_ONLY=1"

if not exist "%VENV_PYTHON%" (
    echo [INFO] Creating Python 3.12 virtual environment...
    py -3.12 -m venv "%VENV_DIR%"
    if errorlevel 1 (
        echo [ERROR] Python 3.12 x64 is required.
        echo         Install it, then run this script again.
        goto :fail
    )
)

"%VENV_PYTHON%" -c "import sys; raise SystemExit(0 if sys.version_info[:2] == (3, 12) else 1)"
if errorlevel 1 (
    echo [WARN] The existing .venv is unusable or does not use Python 3.12.
    echo [INFO] Repairing Python 3.12 virtual environment...
    py -3.12 -m venv --upgrade "%VENV_DIR%"
    if errorlevel 1 (
        echo [ERROR] Failed to repair the Python 3.12 virtual environment.
        echo         Rename or remove .venv manually, then run this script again.
        goto :fail
    )

    "%VENV_PYTHON%" -c "import sys; raise SystemExit(0 if sys.version_info[:2] == (3, 12) else 1)"
    if errorlevel 1 (
        echo [ERROR] The repaired .venv still cannot run Python 3.12.
        echo         Rename or remove .venv manually, then run this script again.
        goto :fail
    )
)

if defined CHECK_ONLY (
    echo [INFO] Environment check completed.
    goto :success
)

"%VENV_PYTHON%" -c "from importlib.metadata import version; expected={'streamlit':'1.60.0','pandas':'3.0.5','plotly':'6.9.0'}; raise SystemExit(0 if all(version(name) == wanted for name, wanted in expected.items()) else 1)" >nul 2>&1
if errorlevel 1 (
    echo [INFO] Installing dashboard dependencies...
    "%VENV_PYTHON%" -m pip install -r requirements.txt
    if errorlevel 1 (
        echo [ERROR] Failed to install dashboard dependencies.
        goto :fail
    )
)

echo [INFO] Starting ParallelPix Performance Dashboard...
echo [INFO] Press Ctrl+C in this window to stop it.
"%VENV_PYTHON%" -m streamlit run tools\dashboard.py
set "DASHBOARD_EXIT=%ERRORLEVEL%"

if not "%DASHBOARD_EXIT%"=="0" (
    echo [ERROR] Dashboard exited with code %DASHBOARD_EXIT%.
    goto :fail
)

:success
popd
endlocal
exit /b 0

:fail
popd
echo.
if not defined CHECK_ONLY pause
endlocal
exit /b 1
