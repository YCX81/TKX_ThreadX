#==============================================================================
# build_crun.ps1 - C-RUN 运行时分析构建 / C-RUN Runtime Analysis Build
#==============================================================================
<#
.SYNOPSIS
    Build with C-RUN runtime checking enabled for safety validation.

.DESCRIPTION
    This script builds the project with IAR C-RUN enabled for runtime
    analysis including bounds checking, overflow detection, etc.

    Note: Requires C-RUN license and Debug configuration.

.PARAMETER IARPath
    Path to IAR EWARM installation

.PARAMETER ProjectPath
    Path to the .ewp project file

.EXAMPLE
    .\build_crun.ps1
#>

param(
    [string]$IARPath = "D:\iar\ewarm-9.70.1",
    [string]$ProjectPath = ""
)

$ErrorActionPreference = "Stop"

$Script:SCRIPT_DIR = $PSScriptRoot
$Script:PROJECT_ROOT = (Get-Item "$Script:SCRIPT_DIR\..\..").FullName

# 使用 Debug 配置（C-RUN 需要调试信息）
# Use Debug configuration (C-RUN requires debug info)
$Configuration = "TKX_ThreadX"

if (-not $ProjectPath) {
    $ProjectPath = Join-Path $Script:PROJECT_ROOT "EWARM\TKX_ThreadX.ewp"
}

Write-Host "=========================================="
Write-Host "C-RUN 运行时分析构建 / C-RUN Build"
Write-Host "=========================================="
Write-Host ""
Write-Host "注意 / Note:"
Write-Host "  C-RUN 检查项需要在 IAR 项目选项中启用："
Write-Host "  Project → Options → C/C++ Compiler → Runtime Checking"
Write-Host ""
Write-Host "  已启用的检查 / Enabled checks:"
Write-Host "    ☑ Bounds checking"
Write-Host "    ☑ Integer overflow"
Write-Host "    ☑ Unsigned overflow"
Write-Host "    ☑ Shift overflow"
Write-Host "    ☑ Division by zero"
Write-Host ""
Write-Host "=========================================="

# 调用主构建脚本
$buildScript = Join-Path $Script:SCRIPT_DIR "build.ps1"

& $buildScript `
    -Configuration $Configuration `
    -IARPath $IARPath `
    -ProjectPath $ProjectPath `
    -Rebuild `
    -Verbose

Write-Host ""
Write-Host "=========================================="
Write-Host "C-RUN 构建完成 / C-RUN Build Complete"
Write-Host ""
Write-Host "下一步 / Next steps:"
Write-Host "  1. 在 IAR 中下载到目标板"
Write-Host "  2. 启动调试器运行测试"
Write-Host "  3. C-RUN 会在运行时检测错误"
Write-Host "=========================================="
