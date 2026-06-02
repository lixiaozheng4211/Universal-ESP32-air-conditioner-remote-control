param(
    [string]$Variant = "Debug"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $root
$releaseDir = Join-Path $repoRoot "release\Android_apk"
New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null

$variantTask = if ($Variant -ieq "Release") { "assembleRelease" } else { "assembleDebug" }
$apkName = if ($Variant -ieq "Release") { "app-release.apk" } else { "app-debug.apk" }

Push-Location $root
try {
    if (Test-Path ".\gradlew.bat") {
        & ".\gradlew.bat" $variantTask
    } elseif (Test-Path ".\.gradle-local\gradle-9.0.0\bin\gradle.bat") {
        & ".\.gradle-local\gradle-9.0.0\bin\gradle.bat" --no-daemon $variantTask
    } else {
        & "gradle" $variantTask
    }
    $apk = Join-Path $root "app\build\outputs\apk\$($Variant.ToLowerInvariant())\$apkName"
    if (!(Test-Path $apk)) {
        throw "APK not found: $apk"
    }
    Copy-Item -Force $apk (Join-Path $releaseDir $apkName)
    Write-Host "APK copied to $releaseDir"
} finally {
    Pop-Location
}
