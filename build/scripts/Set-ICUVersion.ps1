<#
.SYNOPSIS
    This sets environment variable(s) for the ICU version in the build pipeline.

.PARAMETER icuVersionFile
    The path to the file that contains the ICU version.

.EXAMPLE
    Set-ICUVersion -icuVersionFile "C:\icu\version.txt"
#>
param(
    [ValidateScript({Test-Path $_ -PathType 'Leaf'})]
    [Parameter(Mandatory)]
    [string]$icuVersionFile
)

$icuVersionData = Get-Content $icuVersionFile -Raw | ConvertFrom-StringData

if (!$icuVersionData.ICU_version) {
    throw "Error: The ICU Version is empty! (using the file: $icuVersionFile)";
}

Write-Host "ICU Version =" $icuVersionData.ICU_version
$env:ICUVersion = $icuVersionData.ICU_version

# The Azure DevOps environment is a bit odd and requires doing the following
# in order to persist variables from one build task to another build task.
$vstsCommandString = "vso[task.setvariable variable=ICUVersion]" + $icuVersionData.ICU_version
Write-Host "##$vstsCommandString"

# Sanity check on the ICU version number
$icuVersionArray = $env:ICUVersion.split('.')
if ($icuVersionArray.length -ne 4) {
    Write-Host "Error: The ICU version number ($env:ICUVersion) should have exactly 4 parts!".
    throw "Error: Invalid ICU version number!"
}
foreach ($versionPart in $icuVersionArray) {
    # Each part of the ICU version must fit within a uint8_t, so the max value for each part is 255.
    if ([int]$versionPart -gt [int]255) {
        Write-Host "Error: $versionPart exceeds 255! The ICU version number must fit within uint8_t".
        throw "Error: Invalid ICU version number part!"
    }
}

# Edit the source file uvernum.h to set the macro U_ICU_VERSION_BUILDLEVEL_NUM
# to the build-level version read from the version.txt above.
$icuVersionHeader = (Get-Content "$PSScriptRoot\..\..\icu\icu4c\source\common\unicode\uvernum.h")
$icuVersionHeader = $icuVersionHeader.replace('#define U_ICU_VERSION_BUILDLEVEL_NUM 0', '#define U_ICU_VERSION_BUILDLEVEL_NUM ' + $icuVersionArray[3])
$icuVersionHeader | Set-Content "$PSScriptRoot\..\..\icu\icu4c\source\common\unicode\uvernum.h"

# Set the environment variable for the Nuget package to be the same as the ICU version (ex: 64.2.0.1)
Write-Host "Nuget Version = " $env:ICUVersion
$vstsCommandString = "vso[task.setvariable variable=nugetPackageVersion]" + $env:ICUVersion
Write-Host "##$vstsCommandString"
