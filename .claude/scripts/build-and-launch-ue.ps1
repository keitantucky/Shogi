# UEプロジェクトをビルドし、成功時のみ Unreal Editor を起動する
# /build-ue カスタムコマンドから呼び出される想定
# 標準出力はそのまま Claude が読み取れる人間向けの結果メッセージ（1行目に SKIP/SUCCESS/SUCCESS_NO_LAUNCH/FAILURE のいずれかを付ける）

$ErrorActionPreference = 'Stop'

# ===== 環境設定（このマシン・このプロジェクト用に書き換え済み） =====
$EngineRoot    = "C:\Program Files\Epic Games\UE_5.6"
$ProjectName   = "Shogi"
$BuildTarget   = "ShogiEditor"
$Platform      = "Win64"
$Configuration = "Development"
# ================================================================

$ProjectRoot     = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$LogDir          = Join-Path $ProjectRoot "Saved\BuildLogs"
$LogFile         = Join-Path $LogDir "auto_build.log"
$UprojectPath    = Join-Path $ProjectRoot "$ProjectName.uproject"
$BuildBat        = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$UnrealEditorExe = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"

function Write-Log([string]$Message) {
    try {
        if (-not (Test-Path -LiteralPath $LogDir)) {
            New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
        }
        $line = "{0} [build] {1}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $Message
        Add-Content -LiteralPath $LogFile -Value $line -Encoding UTF8
    } catch {}
}

function Test-EditorRunning {
    # このプロジェクトの .uproject を開いている UnrealEditor プロセスがあるかを判定する
    # コマンドライン取得に失敗した場合は安全側に倒し「起動中」とみなす
    try {
        $procs = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" -ErrorAction Stop
        foreach ($p in $procs) {
            if ($p.CommandLine -and $p.CommandLine -like "*$UprojectPath*") {
                return $true
            }
        }
        return $false
    } catch {
        Write-Log "プロセス確認に失敗したため起動中とみなします: $($_.Exception.Message)"
        return $true
    }
}

if (Test-EditorRunning) {
    Write-Log "Editor 起動中のためビルドをスキップ"
    Write-Output "SKIP: Unreal Editor が起動中のため自動ビルドは行いません。先に Editor を閉じてから再度実行してください。"
    exit 0
}

if (-not (Test-Path -LiteralPath $BuildBat) -or -not (Test-Path -LiteralPath $UprojectPath)) {
    Write-Log "ビルド環境が不足 BuildBat=$BuildBat UprojectPath=$UprojectPath"
    Write-Output "SKIP: ビルドに必要な環境が見つからないため自動ビルドは行いません。(BuildBat=$BuildBat / Uproject=$UprojectPath)"
    exit 0
}

Write-Log "ビルド開始: $BuildTarget $Platform $Configuration"
$buildArgs = @($BuildTarget, $Platform, $Configuration, $UprojectPath, "-WaitMutex")
$buildOutput = & $BuildBat @buildArgs 2>&1
$exitCode = $LASTEXITCODE
Write-Log "ビルド終了 (ExitCode=$exitCode)"
if ($buildOutput) {
    Add-Content -LiteralPath $LogFile -Value ($buildOutput | Out-String) -Encoding UTF8
}

if ($exitCode -ne 0) {
    Write-Output "FAILURE: 自動ビルドに失敗しました。(ExitCode=$exitCode) 詳細は $LogFile を確認してください。"
    exit 0
}

if (Test-EditorRunning) {
    Write-Log "ビルド後に Editor 起動を確認（既に起動中）のため新規起動しない"
    Write-Output "SUCCESS_NO_LAUNCH: 自動ビルドは完了しましたが、既に Unreal Editor が起動していたため新規起動は行いませんでした。"
    exit 0
}

if (-not (Test-Path -LiteralPath $UnrealEditorExe)) {
    Write-Log "UnrealEditor.exe が見つからないため起動できません: $UnrealEditorExe"
    Write-Output "SUCCESS_NO_LAUNCH: 自動ビルドは完了しましたが、UnrealEditor.exe が見つからず起動できませんでした。($UnrealEditorExe)"
    exit 0
}

Start-Process -FilePath $UnrealEditorExe -ArgumentList "`"$UprojectPath`""
Write-Log "Unreal Editor を起動しました"
Write-Output "SUCCESS: 自動ビルドが完了し、Unreal Editor を起動しました。"
