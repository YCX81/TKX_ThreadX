#==============================================================================
# build_bootloader.ps1 - Bootloader 构建脚本 / Bootloader Build Script
#==============================================================================
<#
.SYNOPSIS
    Build Bootloader project using the main build script.

.PARAMETER IARPath
    Path to IAR EWARM installation

.PARAMETER Clean
    Perform clean before build

.PARAMETER Rebuild
    Perform full rebuild

.EXAMPLE
    .\build_bootloader.ps1 -Rebuild
#>

param(
    [string]$IARPath = "D:\iar\ewarm-9.70.1",
    [switch]$Clean,
    [switch]$Rebuild
)

$ErrorActionPreference = "Stop"

$Script:SCRIPT_DIR = $PSScriptRoot
$Script:PROJECT_ROOT = (Get-Item "$Script:SCRIPT_DIR\..\..").FullName

# Bootloader 项目路径 / Bootloader project path
$bootloaderProject = Join-Path $Script:PROJECT_ROOT "Bootloader\EWARM\Bootloader.ewp"

# 构建参数 / Build arguments
$buildArgs = @{
    Configuration = "Bootloader"
    IARPath = $IARPath
    ProjectPath = $bootloaderProject
    GenerateHex = $true
    GenerateBin = $true
}

if ($Clean) { $buildArgs.Clean = $true }
if ($Rebuild) { $buildArgs.Rebuild = $true }

# 调用主构建脚本 / Call main build script
$buildScript = Join-Path $Script:SCRIPT_DIR "build.ps1"

& $buildScript @buildArgs
