<#
.SYNOPSIS
    The ICU Visual Studio solution uses "Win32" for the 32-bit arch, but we
    use x86 in the build pipeline.
#>
$IcuArch = "$(BuildPlatform)"
If ($IcuArch -Eq 'x86') {
    $IcuArch = 'Win32'
}
Write-Host "##vso[task.setvariable variable=IcuBuildPlatform]${IcuArch}"

# The ICU ARM builds need to build x64 first, so we need to set this flag
# so we can conditionally build it beforehand.
If (($IcuArch -Eq 'ARM') -or ($IcuArch -Eq 'ARM64')) {
    Write-Host "##vso[task.setvariable variable=BuildingArm]True"
}
