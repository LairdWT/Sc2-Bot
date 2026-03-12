import argparse
import os
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path


def WriteLogLine(LogFileHandle, LineValue: str) -> None:
    print(LineValue)
    LogFileHandle.write(LineValue + "\n")
    LogFileHandle.flush()


def ParseArguments() -> argparse.Namespace:
    ArgumentParser = argparse.ArgumentParser()
    ArgumentParser.add_argument("--repository-root", required=True)
    ArgumentParser.add_argument("--log-path", required=True)
    ArgumentParser.add_argument("--screenshot-path", default="")
    ArgumentParser.add_argument("--test-filter", default="sc2::TestAbilityRemap")
    ArgumentParser.add_argument("--sample-seconds", type=int, default=30)
    return ArgumentParser.parse_args()


def ResolveAllTestsExecutablePath(RepositoryRootPath: Path) -> Path:
    BuildDirectoryPath = RepositoryRootPath / "out" / "build"
    CandidateExecutables = [
        CandidatePath
        for CandidatePath in BuildDirectoryPath.rglob("all_tests.exe")
        if CandidatePath.parent.name.lower() == "bin"
    ]

    if not CandidateExecutables:
        raise FileNotFoundError(f"Could not find all_tests.exe under {BuildDirectoryPath}")

    return max(CandidateExecutables, key=lambda CandidatePath: CandidatePath.stat().st_mtime)


def RunCommand(CommandArguments, TimeoutSeconds: int = 10) -> str:
    try:
        CompletedProcess = subprocess.run(
            CommandArguments,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=TimeoutSeconds,
            check=False,
        )
        OutputText = (CompletedProcess.stdout or "") + (CompletedProcess.stderr or "")
        return OutputText.strip()
    except subprocess.TimeoutExpired:
        return "<command timed out>"


def CaptureProcessState(ProcessNameValue: str) -> str:
    PowerShellCommand = (
        f"Get-Process {ProcessNameValue} -ErrorAction SilentlyContinue | "
        "Select-Object Id, ProcessName, MainWindowTitle, Responding, Path | "
        "Format-List"
    )
    OutputText = RunCommand(["powershell", "-NoProfile", "-Command", PowerShellCommand])
    return OutputText if OutputText else "<process not found>"


def CaptureNetstat(PortValue: int) -> str:
    NetstatOutput = RunCommand(["netstat", "-ano"], TimeoutSeconds=10)
    MatchingLines = [
        OutputLine
        for OutputLine in NetstatOutput.splitlines()
        if f":{PortValue}" in OutputLine
    ]
    return "\n".join(MatchingLines) if MatchingLines else "<no matching netstat entries>"


def CaptureDesktopScreenshot(ScreenshotPath: Path) -> str:
    EscapedScreenshotPathValue = str(ScreenshotPath).replace("'", "''")
    PowerShellCommand = (
        "Add-Type -AssemblyName System.Windows.Forms; "
        "Add-Type -AssemblyName System.Drawing; "
        "$ScreenBounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds; "
        "$BitmapValue = New-Object System.Drawing.Bitmap $ScreenBounds.Width, $ScreenBounds.Height; "
        "$GraphicsValue = [System.Drawing.Graphics]::FromImage($BitmapValue); "
        "$GraphicsValue.CopyFromScreen($ScreenBounds.Location, [System.Drawing.Point]::Empty, $ScreenBounds.Size); "
        f"$BitmapValue.Save('{EscapedScreenshotPathValue}', [System.Drawing.Imaging.ImageFormat]::Png); "
        "$GraphicsValue.Dispose(); "
        "$BitmapValue.Dispose()"
    )
    return RunCommand(["powershell", "-NoProfile", "-Command", PowerShellCommand], TimeoutSeconds=15)


def TerminateProcessesByName(ProcessNames):
    for ProcessNameValue in ProcessNames:
        ExecutableName = ProcessNameValue if ProcessNameValue.lower().endswith(".exe") else ProcessNameValue + ".exe"
        subprocess.run(
            ["taskkill", "/IM", ExecutableName, "/T", "/F"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )


def Main() -> int:
    ParsedArguments = ParseArguments()

    RepositoryRootPath = Path(ParsedArguments.repository_root).resolve()
    LogPath = Path(ParsedArguments.log_path)
    LogPath.parent.mkdir(parents=True, exist_ok=True)

    try:
        AllTestsExecutablePath = ResolveAllTestsExecutablePath(RepositoryRootPath)
    except FileNotFoundError as FileNotFoundErrorValue:
        print(str(FileNotFoundErrorValue), file=sys.stderr)
        return 1

    EnvironmentValues = os.environ.copy()
    EnvironmentValues["SC2_TEST_FILTER"] = ParsedArguments.test_filter
    EnvironmentValues["SC2_WAIT_ON_EXIT"] = "0"

    with LogPath.open("w", encoding="ascii", errors="replace") as LogFileHandle:
        WriteLogLine(LogFileHandle, f"[diagnose] StartTime: {datetime.now().isoformat(timespec='seconds')}")
        WriteLogLine(LogFileHandle, f"[diagnose] TestFilter: {ParsedArguments.test_filter}")
        WriteLogLine(LogFileHandle, f"[diagnose] SampleSeconds: {ParsedArguments.sample_seconds}")
        WriteLogLine(LogFileHandle, f"[diagnose] Executable: {AllTestsExecutablePath}")
        ScreenshotCapturedValue = False

        ProcessHandle = subprocess.Popen(
            [str(AllTestsExecutablePath)],
            cwd=str(AllTestsExecutablePath.parent),
            env=EnvironmentValues,
            stdout=LogFileHandle,
            stderr=subprocess.STDOUT,
        )

        try:
            for SampleIndex in range(ParsedArguments.sample_seconds):
                if ProcessHandle.poll() is not None:
                    break

                time.sleep(1.0)
                WriteLogLine(LogFileHandle, f"[diagnose] SampleIndex: {SampleIndex + 1}")
                WriteLogLine(LogFileHandle, "[diagnose] SC2_x64 process state:")
                WriteLogLine(LogFileHandle, CaptureProcessState("SC2_x64"))
                WriteLogLine(LogFileHandle, "[diagnose] BlizzardError process state:")
                WriteLogLine(LogFileHandle, CaptureProcessState("BlizzardError"))
                WriteLogLine(LogFileHandle, "[diagnose] WerFault process state:")
                WriteLogLine(LogFileHandle, CaptureProcessState("WerFault"))
                WriteLogLine(LogFileHandle, "[diagnose] Netstat 8167:")
                WriteLogLine(LogFileHandle, CaptureNetstat(8167))
                if ParsedArguments.screenshot_path and not ScreenshotCapturedValue:
                    ScreenshotPath = Path(ParsedArguments.screenshot_path)
                    ScreenshotPath.parent.mkdir(parents=True, exist_ok=True)
                    CaptureDesktopScreenshot(ScreenshotPath)
                    WriteLogLine(LogFileHandle, f"[diagnose] ScreenshotPath: {ScreenshotPath}")
                    ScreenshotCapturedValue = True

            ExitCodeValue = ProcessHandle.poll()
            if ExitCodeValue is None:
                WriteLogLine(LogFileHandle, "[diagnose] Sample window expired. Terminating test and SC2 helper processes.")
                TerminateProcessesByName(["all_tests", "SC2_x64", "BlizzardError", "WerFault"])
                return 124

            WriteLogLine(LogFileHandle, f"[diagnose] all_tests exit code: {ExitCodeValue}")
            return int(ExitCodeValue)
        finally:
            TerminateProcessesByName(["SC2_x64", "BlizzardError", "WerFault"])


if __name__ == "__main__":
    sys.exit(Main())
