param([Parameter(Mandatory=$true)][string]$Driver)
$ErrorActionPreference = 'Stop'
$root = Join-Path ([System.IO.Path]::GetTempPath()) ("bluewake-crash-test-" + [guid]::NewGuid())
New-Item -ItemType Directory (Join-Path $root 'logs') | Out-Null
try {
    foreach ($mode in @('null', 'abort', 'terminate')) {
        $env:BLUEWAKE_CRASH_TEST = $mode
        $process = Start-Process -FilePath $Driver -ArgumentList ($root + '\') -Wait -PassThru -NoNewWindow
        if ($process.ExitCode -eq 0) { throw "$mode unexpectedly succeeded" }
        $report = Get-Content (Join-Path $root "logs\crash-$($process.Id).log") -Raw
        if ($report -notmatch '\[crash\] frame=') { throw "$mode lacks a stack" }
        if ($mode -eq 'null' -and $report -notmatch 'null-call caller=0x[0-9a-f]*[1-9a-f]') { throw 'Null caller missing' }
        if ($mode -eq 'abort' -and $report -notmatch 'SIGABRT') { throw 'Abort reason missing' }
        if ($mode -eq 'terminate' -and $report -notmatch 'std::terminate') { throw 'Terminate reason missing' }
    }
} finally {
    Remove-Item Env:BLUEWAKE_CRASH_TEST -ErrorAction SilentlyContinue
    Remove-Item -Recurse $root
}
