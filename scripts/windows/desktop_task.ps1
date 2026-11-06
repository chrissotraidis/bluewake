<#
  Run a command in the desktop session, from SSH, and wait for it.

      powershell -ExecutionPolicy Bypass -File scripts\windows\desktop_task.ps1 -Command C:\bench\run1.cmd [-Name render-bench]

  An SSH login runs in session 0, which has no desktop. A scheduled task with an Interactive principal runs in the
  session of the user logged on at the desktop instead: log in over SSH as that user. Normal priority (4): a task's default is below normal,
  which every process it starts inherits. The task is deleted when the command ends.
#>
param(
    [Parameter(Mandatory)] [string]$Command,
    [string]$Name = "render-bench"
)
$ErrorActionPreference = "Stop"
$action = New-ScheduledTaskAction -Execute $Command
# This account (over SSH, $env:USERDOMAIN is WORKGROUP, not the PC's name).
$user = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
$principal = New-ScheduledTaskPrincipal -UserId $user -LogonType Interactive -RunLevel Limited
$settings = New-ScheduledTaskSettingsSet -Priority 4 -ExecutionTimeLimit ([TimeSpan]::Zero) `
    -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -MultipleInstances IgnoreNew
Register-ScheduledTask -TaskName $Name -Action $action -Principal $principal -Settings $settings -Force | Out-Null
try {
    Start-ScheduledTask -TaskName $Name
    Start-Sleep 2
    while ((Get-ScheduledTask -TaskName $Name).State -eq "Running") { Start-Sleep 5 }
    $result = (Get-ScheduledTaskInfo -TaskName $Name).LastTaskResult
    Write-Output "$Name ended: $result"
    exit $result
} finally {
    Unregister-ScheduledTask -TaskName $Name -Confirm:$false
}
