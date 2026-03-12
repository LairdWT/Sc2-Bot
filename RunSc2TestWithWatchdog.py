import argparse
import ctypes
import os
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Sequence

if os.name == "nt":
    from ctypes import wintypes

    class FIoCounters(ctypes.Structure):
        _fields_ = [
            ("ReadOperationCount", ctypes.c_ulonglong),
            ("WriteOperationCount", ctypes.c_ulonglong),
            ("OtherOperationCount", ctypes.c_ulonglong),
            ("ReadTransferCount", ctypes.c_ulonglong),
            ("WriteTransferCount", ctypes.c_ulonglong),
            ("OtherTransferCount", ctypes.c_ulonglong),
        ]

    class FJobObjectBasicLimitInformation(ctypes.Structure):
        _fields_ = [
            ("PerProcessUserTimeLimit", ctypes.c_longlong),
            ("PerJobUserTimeLimit", ctypes.c_longlong),
            ("LimitFlags", wintypes.DWORD),
            ("MinimumWorkingSetSize", ctypes.c_size_t),
            ("MaximumWorkingSetSize", ctypes.c_size_t),
            ("ActiveProcessLimit", wintypes.DWORD),
            ("Affinity", ctypes.c_size_t),
            ("PriorityClass", wintypes.DWORD),
            ("SchedulingClass", wintypes.DWORD),
        ]

    class FJobObjectExtendedLimitInformation(ctypes.Structure):
        _fields_ = [
            ("BasicLimitInformation", FJobObjectBasicLimitInformation),
            ("IoInfo", FIoCounters),
            ("ProcessMemoryLimit", ctypes.c_size_t),
            ("JobMemoryLimit", ctypes.c_size_t),
            ("PeakProcessMemoryUsed", ctypes.c_size_t),
            ("PeakJobMemoryUsed", ctypes.c_size_t),
        ]

    KERNEL32 = ctypes.WinDLL("kernel32", use_last_error=True)
    JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x00002000
    JOB_OBJECT_EXTENDED_LIMIT_INFORMATION = 9


def WriteLogLine(LogFileHandle, Line: str) -> None:
    print(Line)
    LogFileHandle.write(Line + "\n")
    LogFileHandle.flush()


def TerminateProcessTree(ProcessId: int) -> None:
    subprocess.run(
        ["taskkill", "/PID", str(ProcessId), "/T", "/F"],
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def TerminateProcessesByName(ProcessNames: Sequence[str]) -> None:
    for ProcessName in ProcessNames:
        ExecutableName = ProcessName if ProcessName.lower().endswith(".exe") else ProcessName + ".exe"
        subprocess.run(
            ["taskkill", "/IM", ExecutableName, "/T", "/F"],
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )


def ParseArguments() -> argparse.Namespace:
    ArgumentParser = argparse.ArgumentParser()
    ExecutableSourceGroup = ArgumentParser.add_mutually_exclusive_group(required=True)
    ExecutableSourceGroup.add_argument("--executable-path")
    ExecutableSourceGroup.add_argument("--repository-root")
    ArgumentParser.add_argument("--log-path", required=True)
    ArgumentParser.add_argument("--timeout-seconds", type=int, default=600)
    ArgumentParser.add_argument("--test-filter", default="")
    ArgumentParser.add_argument("--append", action="store_true")
    ArgumentParser.add_argument(
        "--watchdog-process-name",
        action="append",
        default=["all_tests", "SC2_x64"],
    )
    ArgumentParser.add_argument("command_arguments", nargs="*")
    return ArgumentParser.parse_args()


def ResolveExecutablePath(ParsedArguments: argparse.Namespace) -> Path:
    if ParsedArguments.executable_path:
        return Path(ParsedArguments.executable_path).resolve()

    RepositoryRoot = Path(ParsedArguments.repository_root).resolve()
    BuildDirectory = RepositoryRoot / "out" / "build"
    CandidateExecutables = [
        CandidatePath
        for CandidatePath in BuildDirectory.rglob("all_tests.exe")
        if CandidatePath.parent.name.lower() == "bin"
    ]

    if not CandidateExecutables:
        raise FileNotFoundError(f"Could not find all_tests.exe under {BuildDirectory}")

    return max(CandidateExecutables, key=lambda CandidatePath: CandidatePath.stat().st_mtime)


def ConfigureWindowsErrorMode() -> None:
    if os.name != "nt":
        return

    SemFailCriticalErrors = 0x0001
    SemNoGpFaultErrorBox = 0x0002
    SemNoOpenFileErrorBox = 0x8000
    ctypes.windll.kernel32.SetErrorMode(
        SemFailCriticalErrors | SemNoGpFaultErrorBox | SemNoOpenFileErrorBox
    )


def CreateKillOnCloseJobHandle() -> int | None:
    if os.name != "nt":
        return None

    JobHandle = KERNEL32.CreateJobObjectW(None, None)
    if not JobHandle:
        raise ctypes.WinError(ctypes.get_last_error())

    ExtendedLimitInformation = FJobObjectExtendedLimitInformation()
    ExtendedLimitInformation.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE

    SetInformationSucceeded = KERNEL32.SetInformationJobObject(
        JobHandle,
        JOB_OBJECT_EXTENDED_LIMIT_INFORMATION,
        ctypes.byref(ExtendedLimitInformation),
        ctypes.sizeof(ExtendedLimitInformation),
    )
    if not SetInformationSucceeded:
        KERNEL32.CloseHandle(JobHandle)
        raise ctypes.WinError(ctypes.get_last_error())

    return int(JobHandle)


def AttachProcessToKillOnCloseJob(LogFileHandle, Process: subprocess.Popen) -> int | None:
    if os.name != "nt":
        return None

    JobHandle = CreateKillOnCloseJobHandle()
    AssignSucceeded = KERNEL32.AssignProcessToJobObject(JobHandle, wintypes.HANDLE(Process._handle))
    if not AssignSucceeded:
        LastError = ctypes.get_last_error()
        KERNEL32.CloseHandle(JobHandle)
        WriteLogLine(
            LogFileHandle,
            f"[watchdog] Warning: failed to attach process to job object (WinError {LastError}).",
        )
        return None

    WriteLogLine(LogFileHandle, "[watchdog] Attached test runner to kill-on-close job object.")
    return JobHandle


def CloseWindowsHandle(HandleValue: int | None) -> None:
    if os.name != "nt" or HandleValue is None:
        return

    KERNEL32.CloseHandle(wintypes.HANDLE(HandleValue))


def Main() -> int:
    ParsedArguments = ParseArguments()

    try:
        ResolvedExecutablePath = ResolveExecutablePath(ParsedArguments)
    except FileNotFoundError as FileNotFoundErrorException:
        print(str(FileNotFoundErrorException), file=sys.stderr)
        return 1
    ResolvedLogPath = Path(ParsedArguments.log_path)
    ResolvedLogPath.parent.mkdir(parents=True, exist_ok=True)

    OpenMode = "a" if ParsedArguments.append else "w"
    Environment = os.environ.copy()
    if ParsedArguments.test_filter:
        Environment["SC2_TEST_FILTER"] = ParsedArguments.test_filter

    Command = [str(ResolvedExecutablePath), *ParsedArguments.command_arguments]
    ConfigureWindowsErrorMode()

    with ResolvedLogPath.open(OpenMode, encoding="ascii", errors="replace") as LogFileHandle:
        StartTimestamp = datetime.now().isoformat(timespec="seconds")
        WriteLogLine(LogFileHandle, f"[watchdog] StartTime: {StartTimestamp}")
        WriteLogLine(LogFileHandle, f"[watchdog] Starting: {ResolvedExecutablePath}")
        WriteLogLine(LogFileHandle, f"[watchdog] TimeoutSeconds: {ParsedArguments.timeout_seconds}")
        if ParsedArguments.test_filter:
            WriteLogLine(LogFileHandle, f"[watchdog] SC2_TEST_FILTER: {ParsedArguments.test_filter}")

        JobHandle = None
        try:
            Process = subprocess.Popen(
                Command,
                cwd=str(ResolvedExecutablePath.parent),
                env=Environment,
                stdout=LogFileHandle,
                stderr=subprocess.STDOUT,
            )
            JobHandle = AttachProcessToKillOnCloseJob(LogFileHandle, Process)

            DeadlineTimestamp = time.monotonic() + ParsedArguments.timeout_seconds
            ExitCode = None
            while ExitCode is None:
                ExitCode = Process.poll()
                if ExitCode is not None:
                    break

                if time.monotonic() >= DeadlineTimestamp:
                    WriteLogLine(LogFileHandle, "[watchdog] Timeout reached. Terminating test and SC2 processes.")
                    TerminateProcessTree(Process.pid)
                    TerminateProcessesByName(ParsedArguments.watchdog_process_name)
                    return 124

                time.sleep(1.0)

            WriteLogLine(LogFileHandle, f"[watchdog] ExitCode: {ExitCode}")
            return int(ExitCode)
        finally:
            CloseWindowsHandle(JobHandle)


if __name__ == "__main__":
    sys.exit(Main())
