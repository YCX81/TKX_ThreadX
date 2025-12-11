#==============================================================================
# build.ps1 - TKX_ThreadX 自动化构建脚本 / Automated Build Script
# 功能安全等级 / Safety Level: IEC 61508 SIL 2
#==============================================================================
<#
.SYNOPSIS
    Build TKX_ThreadX project using IAR EWARM command line.

.DESCRIPTION
    This script automates the build process for the TKX_ThreadX project.
    It supports clean, rebuild, and incremental builds, and can generate
    HEX/BIN output files.

.PARAMETER Configuration
    Build configuration name (default: TKX_ThreadX)

.PARAMETER IARPath
    Path to IAR EWARM installation

.PARAMETER ProjectPath
    Path to the .ewp project file

.PARAMETER Clean
    Perform clean before build

.PARAMETER Rebuild
    Perform full rebuild

.PARAMETER GenerateHex
    Generate Intel HEX output file

.PARAMETER GenerateBin
    Generate binary output file

.PARAMETER Verbose
    Enable verbose build output

.EXAMPLE
    .\build.ps1 -Rebuild -GenerateHex -GenerateBin
    .\build.ps1 -Configuration "TKX_ThreadX" -Clean
#>

param(
    [string]$Configuration = "TKX_ThreadX",
    [string]$IARPath = "D:\iar\ewarm-9.70.1",
    [string]$ProjectPath = "",
    [switch]$Clean,
    [switch]$Rebuild,
    [switch]$GenerateHex,
    [switch]$GenerateBin,
    [switch]$Verbose
)

# 错误处理 / Error handling
$ErrorActionPreference = "Stop"
$Script:ExitCode = 0

#------------------------------------------------------------------------------
# 配置 / Configuration
#------------------------------------------------------------------------------
$Script:IAR_BUILD = Join-Path $IARPath "common\bin\iarbuild.exe"
$Script:IAR_ELF = Join-Path $IARPath "arm\bin\ielftool.exe"
$Script:SCRIPT_DIR = $PSScriptRoot
$Script:PROJECT_ROOT = (Get-Item "$Script:SCRIPT_DIR\..\..").FullName

# 默认项目路径 / Default project path
if (-not $ProjectPath) {
    $ProjectPath = Join-Path $Script:PROJECT_ROOT "EWARM\TKX_ThreadX.ewp"
}

# 解析路径 / Resolve path
if (Test-Path $ProjectPath) {
    $ProjectPath = (Resolve-Path $ProjectPath).Path
}

# 输出目录 / Output directories
$Script:OUTPUT_DIR = Join-Path (Split-Path $ProjectPath -Parent) "$Configuration\Exe"
$Script:ARTIFACT_DIR = Join-Path $Script:PROJECT_ROOT "artifacts"

#------------------------------------------------------------------------------
# 函数定义 / Function Definitions
#------------------------------------------------------------------------------

function Write-BuildLog {
    param(
        [string]$Message,
        [string]$Level = "INFO"
    )

    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $color = switch ($Level) {
        "INFO"    { "White" }
        "SUCCESS" { "Green" }
        "WARNING" { "Yellow" }
        "ERROR"   { "Red" }
        "DEBUG"   { "Cyan" }
        default   { "White" }
    }

    Write-Host "[$timestamp] [$Level] $Message" -ForegroundColor $color
}

function Test-Prerequisites {
    Write-BuildLog "检查构建环境... / Checking build environment..."

    # 检查 IAR iarbuild
    if (-not (Test-Path $Script:IAR_BUILD)) {
        throw "IAR iarbuild.exe 未找到 / not found: $Script:IAR_BUILD"
    }

    # 检查 IAR ielftool
    if (-not (Test-Path $Script:IAR_ELF)) {
        throw "IAR ielftool.exe 未找到 / not found: $Script:IAR_ELF"
    }

    # 检查项目文件
    if (-not (Test-Path $ProjectPath)) {
        throw "项目文件未找到 / Project file not found: $ProjectPath"
    }

    Write-BuildLog "IAR 路径 / IAR Path: $IARPath" "SUCCESS"
    Write-BuildLog "项目文件 / Project: $ProjectPath" "SUCCESS"
}

function Invoke-Build {
    param(
        [string]$BuildAction = "build"
    )

    $buildArgs = @()

    switch ($BuildAction) {
        "clean" {
            $buildArgs = @($ProjectPath, "-clean", $Configuration)
            Write-BuildLog "执行清理... / Cleaning..."
        }
        "rebuild" {
            $buildArgs = @($ProjectPath, "-make", $Configuration)
            Write-BuildLog "执行重新构建... / Rebuilding..."
        }
        default {
            $buildArgs = @($ProjectPath, "-build", $Configuration)
            Write-BuildLog "执行增量构建... / Building..."
        }
    }

    if ($Verbose) {
        $buildArgs += "-log", "all"
    }

    Write-BuildLog "命令 / Command: $Script:IAR_BUILD $($buildArgs -join ' ')" "DEBUG"

    # 执行构建 / Execute build
    $pinfo = New-Object System.Diagnostics.ProcessStartInfo
    $pinfo.FileName = $Script:IAR_BUILD
    $pinfo.Arguments = $buildArgs -join ' '
    $pinfo.RedirectStandardOutput = $true
    $pinfo.RedirectStandardError = $true
    $pinfo.UseShellExecute = $false
    $pinfo.CreateNoWindow = $true
    $pinfo.WorkingDirectory = Split-Path $ProjectPath -Parent

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $pinfo
    $process.Start() | Out-Null

    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    # 输出构建日志 / Output build log
    if ($stdout) {
        $stdout -split "`n" | ForEach-Object {
            $line = $_.Trim()
            if ($line) {
                if ($line -match "Error|错误") {
                    Write-Host $line -ForegroundColor Red
                }
                elseif ($line -match "Warning|警告") {
                    Write-Host $line -ForegroundColor Yellow
                }
                else {
                    Write-Host $line
                }
            }
        }
    }

    if ($stderr) {
        Write-Host $stderr -ForegroundColor Red
    }

    if ($process.ExitCode -ne 0) {
        throw "构建失败 / Build failed, exit code: $($process.ExitCode)"
    }

    Write-BuildLog "构建成功 / Build successful" "SUCCESS"
}

function Convert-ToHexBin {
    # 查找输出文件 / Find output file
    $outFile = Join-Path $Script:OUTPUT_DIR "$Configuration.out"

    if (-not (Test-Path $outFile)) {
        # 尝试其他可能的名称 / Try other possible names
        $outFile = Get-ChildItem -Path $Script:OUTPUT_DIR -Filter "*.out" | Select-Object -First 1
        if (-not $outFile) {
            throw "输出文件未找到 / Output file not found in: $Script:OUTPUT_DIR"
        }
        $outFile = $outFile.FullName
    }

    Write-BuildLog "输出文件 / Output file: $outFile" "DEBUG"

    # 创建 artifacts 目录 / Create artifacts directory
    if (-not (Test-Path $Script:ARTIFACT_DIR)) {
        New-Item -ItemType Directory -Path $Script:ARTIFACT_DIR -Force | Out-Null
    }

    if ($GenerateHex) {
        $hexFile = Join-Path $Script:ARTIFACT_DIR "$Configuration.hex"
        Write-BuildLog "生成 HEX 文件 / Generating HEX: $hexFile"

        $hexArgs = @($outFile, "--ihex", $hexFile)
        $process = Start-Process -FilePath $Script:IAR_ELF -ArgumentList $hexArgs -NoNewWindow -Wait -PassThru

        if ($process.ExitCode -ne 0) {
            throw "HEX 转换失败 / HEX conversion failed"
        }

        $hexSize = (Get-Item $hexFile).Length
        Write-BuildLog "HEX 大小 / HEX size: $hexSize bytes" "SUCCESS"
    }

    if ($GenerateBin) {
        $binFile = Join-Path $Script:ARTIFACT_DIR "$Configuration.bin"
        Write-BuildLog "生成 BIN 文件 / Generating BIN: $binFile"

        $binArgs = @($outFile, "--bin", $binFile)
        $process = Start-Process -FilePath $Script:IAR_ELF -ArgumentList $binArgs -NoNewWindow -Wait -PassThru

        if ($process.ExitCode -ne 0) {
            throw "BIN 转换失败 / BIN conversion failed"
        }

        $binSize = (Get-Item $binFile).Length
        Write-BuildLog "BIN 大小 / BIN size: $binSize bytes" "SUCCESS"

        # 检查大小限制 / Check size limit (448KB for application)
        $maxSize = 448KB
        if ($binSize -gt $maxSize) {
            Write-BuildLog "警告: BIN 文件超过限制 / WARNING: BIN exceeds limit ($maxSize bytes)" "WARNING"
        }
    }
}

function Export-BuildInfo {
    # 获取 Git 信息 / Get Git info
    $gitCommit = ""
    $gitBranch = ""
    $gitTag = ""
    $gitDirty = $false

    try {
        Push-Location $Script:PROJECT_ROOT
        $gitCommit = (git rev-parse --short HEAD 2>$null) -replace "`n", ""
        $gitBranch = (git rev-parse --abbrev-ref HEAD 2>$null) -replace "`n", ""
        $gitTag = (git describe --tags --always 2>$null) -replace "`n", ""
        $gitDirty = [bool](git status --porcelain 2>$null)
        Pop-Location
    }
    catch {
        Write-BuildLog "Git 信息获取失败 / Git info unavailable" "WARNING"
    }

    # 构建信息 / Build info
    $buildInfo = @{
        project = $Configuration
        version = "1.0.0"
        build_number = (Get-Date -Format "yyMMddHHmm")
        timestamp = (Get-Date -Format "yyyy-MM-ddTHH:mm:ssZ")
        git = @{
            commit = $gitCommit
            branch = $gitBranch
            tag = $gitTag
            dirty = $gitDirty
        }
        toolchain = @{
            iar_version = "9.70.1"
            iar_path = $IARPath
        }
        build_host = $env:COMPUTERNAME
        status = "SUCCESS"
    }

    # 保存 JSON / Save JSON
    $buildInfoPath = Join-Path $Script:ARTIFACT_DIR "build_info.json"
    $buildInfo | ConvertTo-Json -Depth 4 | Out-File -FilePath $buildInfoPath -Encoding UTF8

    Write-BuildLog "构建信息已保存 / Build info saved: $buildInfoPath" "SUCCESS"
}

#------------------------------------------------------------------------------
# 主流程 / Main Process
#------------------------------------------------------------------------------
try {
    Write-BuildLog "=========================================="
    Write-BuildLog "TKX_ThreadX 自动化构建 / Automated Build"
    Write-BuildLog "配置 / Configuration: $Configuration"
    Write-BuildLog "=========================================="

    # 检查环境 / Check environment
    Test-Prerequisites

    # 清理 / Clean
    if ($Clean) {
        Invoke-Build "clean"
    }

    # 构建 / Build
    if ($Rebuild) {
        if (-not $Clean) {
            Invoke-Build "clean"
        }
        Invoke-Build "build"
    }
    else {
        Invoke-Build "build"
    }

    # 生成输出文件 / Generate output files
    if ($GenerateHex -or $GenerateBin) {
        Convert-ToHexBin
    }

    # 导出构建信息 / Export build info
    Export-BuildInfo

    Write-BuildLog "=========================================="
    Write-BuildLog "构建完成 / Build Complete" "SUCCESS"
    Write-BuildLog "=========================================="

    exit 0
}
catch {
    Write-BuildLog $_.Exception.Message "ERROR"
    Write-BuildLog "构建失败 / Build Failed" "ERROR"
    exit 1
}
