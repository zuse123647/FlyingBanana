# Builds a .qmod file for loading with QuestPatcher
$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt

$buildScript = "$NDKPath/build/ndk-build"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $buildScript += ".cmd"
}

$ArchiveName = "FlyingBanana_v1.1.0.qmod"
$TempArchiveName = "FlyingBanana_v1.1.0.qmod.zip"

& $buildScript NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk
Compress-Archive -Path "./libs/arm64-v8a/libFlyingBanana.so", "./libs/arm64-v8a/libbeatsaber-hook_1_3_5.so", "./mod.json", "./coverImage.png" -DestinationPath $TempArchiveName -Force
Move-Item $TempArchiveName $ArchiveName -Force