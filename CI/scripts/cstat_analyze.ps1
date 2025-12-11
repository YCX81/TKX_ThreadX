#==============================================================================
# cstat_analyze.ps1 - C-STAT 静态分析脚本 / Static Analysis Script
# MISRA-C:2012 / CERT-C 合规性检查 / Compliance Checking
#==============================================================================
<#
.SYNOPSIS
    Run C-STAT static analysis on TKX_ThreadX project.

.DESCRIPTION
    This script automates C-STAT analysis for MISRA-C compliance checking.
    It excludes third-party code and generates filtered reports.

.PARAMETER Configuration
    Build configuration name

.PARAMETER IARPath
    Path to IAR EWARM installation

.PARAMETER ProjectPath
    Path to the .ewp project file

.PARAMETER OutputFormat
    Report output format (html, xml, txt)

.PARAMETER FailOnHigh
    Fail if high severity issues found

.PARAMETER FailOnMedium
    Fail if medium severity issues found

.EXAMPLE
    .\cstat_analyze.ps1 -FailOnHigh
    .\cstat_analyze.ps1 -OutputFormat html
#>

param(
    [string]$Configuration = "TKX_ThreadX",
    [string]$IARPath = "D:\iar\ewarm-9.70.1",
    [string]$ProjectPath = "",
    [string]$OutputFormat = "html",
    [switch]$FailOnHigh,
    [switch]$FailOnMedium
)

$ErrorActionPreference = "Stop"

#------------------------------------------------------------------------------
# 配置 / Configuration
#------------------------------------------------------------------------------
$Script:IAR_BUILD = Join-Path $IARPath "common\bin\iarbuild.exe"
$Script:SCRIPT_DIR = $PSScriptRoot
$Script:PROJECT_ROOT = (Get-Item "$Script:SCRIPT_DIR\..\..").FullName

# 默认项目路径 / Default project path
if (-not $ProjectPath) {
    $ProjectPath = Join-Path $Script:PROJECT_ROOT "EWARM\TKX_ThreadX.ewp"
}

if (Test-Path $ProjectPath) {
    $ProjectPath = (Resolve-Path $ProjectPath).Path
}

$Script:OUTPUT_DIR = Join-Path $Script:PROJECT_ROOT "artifacts\cstat"

# 排除路径列表 / Excluded paths
$Script:ExcludePaths = @(
    "Middlewares\\ST\\threadx",
    "Middlewares\\ST\\filex",
    "Middlewares\\ST\\levelx",
    "Middlewares\\ST\\usbx",
    "Middlewares\\ST\\netxduo",
    "Middlewares\\Third_Party",
    "Drivers\\CMSIS",
    "ThirdParty\\SEGGER"
)

#------------------------------------------------------------------------------
# 函数定义 / Function Definitions
#------------------------------------------------------------------------------

function Write-Log {
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

function Invoke-CStatAnalysis {
    Write-Log "启动 C-STAT 分析... / Starting C-STAT analysis..."

    # 确保输出目录存在 / Ensure output directory exists
    if (-not (Test-Path $Script:OUTPUT_DIR)) {
        New-Item -ItemType Directory -Path $Script:OUTPUT_DIR -Force | Out-Null
    }

    # 使用 iarbuild 执行 C-STAT / Run C-STAT via iarbuild
    $cstatArgs = @($ProjectPath, "-cstat_analyze", $Configuration)

    Write-Log "命令 / Command: $Script:IAR_BUILD $($cstatArgs -join ' ')" "DEBUG"

    $pinfo = New-Object System.Diagnostics.ProcessStartInfo
    $pinfo.FileName = $Script:IAR_BUILD
    $pinfo.Arguments = $cstatArgs -join ' '
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

    if ($stdout) {
        Write-Log "C-STAT 输出 / C-STAT output:"
        $stdout -split "`n" | ForEach-Object {
            if ($_.Trim()) { Write-Host $_ }
        }
    }

    return $process.ExitCode
}

function Analyze-Results {
    Write-Log "分析 C-STAT 结果... / Analyzing C-STAT results..."

    # 查找 C-STAT 消息文件 / Find C-STAT messages file
    $messagesFile = Join-Path (Split-Path $ProjectPath -Parent) "C-STAT Messages.txt"

    if (-not (Test-Path $messagesFile)) {
        Write-Log "C-STAT 消息文件未找到 / C-STAT messages file not found" "WARNING"
        return @{ High = 0; Medium = 0; Low = 0; Total = 0 }
    }

    # 读取并过滤内容 / Read and filter content
    $content = Get-Content $messagesFile -Encoding UTF8

    # 过滤排除路径 / Filter excluded paths
    $filteredLines = @()
    foreach ($line in $content) {
        $exclude = $false
        foreach ($path in $Script:ExcludePaths) {
            if ($line -match [regex]::Escape($path)) {
                $exclude = $true
                break
            }
        }
        if (-not $exclude) {
            $filteredLines += $line
        }
    }

    # 统计各级别问题 / Count issues by severity
    $stats = @{
        High = 0
        Medium = 0
        Low = 0
        Total = 0
    }

    foreach ($line in $filteredLines) {
        if ($line -match "\tHigh\t") { $stats.High++ }
        elseif ($line -match "\tMedium\t") { $stats.Medium++ }
        elseif ($line -match "\tLow\t") { $stats.Low++ }
    }

    $stats.Total = $stats.High + $stats.Medium + $stats.Low

    # 保存过滤后的报告 / Save filtered report
    $filteredReport = Join-Path $Script:OUTPUT_DIR "C-STAT_Messages_Filtered.txt"
    $filteredLines | Out-File -FilePath $filteredReport -Encoding UTF8

    # 复制原始报告 / Copy original report
    $fullReport = Join-Path $Script:OUTPUT_DIR "C-STAT_Messages_Full.txt"
    Copy-Item $messagesFile $fullReport -Force

    Write-Log "过滤后报告 / Filtered report: $filteredReport" "SUCCESS"

    return $stats
}

function Export-Summary {
    param(
        [hashtable]$Stats
    )

    # 生成 JSON 摘要 / Generate JSON summary
    $summary = @{
        timestamp = (Get-Date -Format "yyyy-MM-ddTHH:mm:ssZ")
        project = $Configuration
        total_issues = $Stats.Total
        high = $Stats.High
        medium = $Stats.Medium
        low = $Stats.Low
        excluded_paths = $Script:ExcludePaths
        compliance = @{
            target = "MISRA-C:2012"
            safety_level = "IEC 61508 SIL 2"
        }
    }

    $summaryPath = Join-Path $Script:OUTPUT_DIR "cstat_summary.json"
    $summary | ConvertTo-Json -Depth 4 | Out-File -FilePath $summaryPath -Encoding UTF8

    Write-Log "摘要已保存 / Summary saved: $summaryPath" "SUCCESS"
}

function Show-Summary {
    param(
        [hashtable]$Stats
    )

    Write-Host ""
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "  C-STAT 分析结果摘要 / Analysis Summary" -ForegroundColor Cyan
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host ""

    $highColor = if ($Stats.High -gt 0) { "Red" } else { "Green" }
    $mediumColor = if ($Stats.Medium -gt 0) { "Yellow" } else { "Green" }

    Write-Host "  高严重度 / High:    " -NoNewline
    Write-Host $Stats.High -ForegroundColor $highColor
    Write-Host "  中严重度 / Medium:  " -NoNewline
    Write-Host $Stats.Medium -ForegroundColor $mediumColor
    Write-Host "  低严重度 / Low:     $($Stats.Low)"
    Write-Host "  ─────────────────────────"
    Write-Host "  总计 / Total:       $($Stats.Total)"
    Write-Host ""
    Write-Host "==========================================" -ForegroundColor Cyan
}

#------------------------------------------------------------------------------
# 主流程 / Main Process
#------------------------------------------------------------------------------
try {
    Write-Log "=========================================="
    Write-Log "C-STAT 静态分析 / Static Analysis"
    Write-Log "项目 / Project: $Configuration"
    Write-Log "=========================================="

    # 检查 IAR / Check IAR
    if (-not (Test-Path $Script:IAR_BUILD)) {
        throw "IAR iarbuild.exe 未找到 / not found: $Script:IAR_BUILD"
    }

    if (-not (Test-Path $ProjectPath)) {
        throw "项目文件未找到 / Project file not found: $ProjectPath"
    }

    # 执行分析 / Run analysis
    $exitCode = Invoke-CStatAnalysis

    # 分析结果 / Analyze results
    $stats = Analyze-Results

    # 导出摘要 / Export summary
    Export-Summary -Stats $stats

    # 显示摘要 / Show summary
    Show-Summary -Stats $stats

    # 检查失败条件 / Check failure conditions
    if ($FailOnHigh -and $stats.High -gt 0) {
        Write-Log "检测到 $($stats.High) 个高严重度问题 / $($stats.High) high severity issues found" "ERROR"
        exit 1
    }

    if ($FailOnMedium -and $stats.Medium -gt 0) {
        Write-Log "检测到 $($stats.Medium) 个中严重度问题 / $($stats.Medium) medium severity issues found" "ERROR"
        exit 1
    }

    Write-Log "C-STAT 分析完成 / Analysis complete" "SUCCESS"
    exit 0
}
catch {
    Write-Log $_.Exception.Message "ERROR"
    exit 1
}
