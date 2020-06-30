<#
.SYNOPSIS
    The ICU Visual Studio solution uses "Win32" for the 32-bit arch, but we
    use x86 in the build pipeline.
#>
$IcuArch = $env:BuildPlatform
If ($IcuArch -Eq 'x86') {
    $IcuArch = 'Win32'
}
$vstsCommandString = 'vso[task.setvariable variable=IcuBuildPlatform]' + $IcuArch
Write-Host "##$vstsCommandString"

# The ICU ARM builds need to build x64 first, so we need to set this flag
# so we can conditionally build it beforehand.
If (($IcuArch -Eq 'ARM') -or ($IcuArch -Eq 'ARM64')) {
    Write-Host '##vso[task.setvariable variable=BuildingArm]True'
}
