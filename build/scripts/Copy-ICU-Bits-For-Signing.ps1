<#
.SYNOPSIS
    This script copies the generated ICU binaries (EXEs and DLLs) to a single folder in order to simplify
    the Code Signing, and also checks that the ICU data DLL isn't the stubdata file.

.PARAMETER source
    Path to the generated ICU binaries. (Likely from an existing build artifact).

.PARAMETER output
    The path to the output folder location for the VPacks. (Will be created).

.PARAMETER arch
    The architecture that ICU was compiled for: x64, x86, ARM, or ARM64.
#>
param(
    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$source,

    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$output,

    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$arch
)

$icuSource = Resolve-Path "$source\icu\icu4c"
$inputBinaries = ""
$inputLibs     = ""

if ($arch -eq "x64") {
    $inputBinaries = "$icuSource\bin64"
    $inputLibs     = "$icuSource\lib64"
} elseif ($arch -eq "x86") {
    $inputBinaries = "$icuSource\bin"
    $inputLibs     = "$icuSource\lib"
} elseif ($arch -eq "ARM") {
    $inputBinaries = "$icuSource\binARM"
    $inputLibs     = "$icuSource\libARM"
} elseif ($arch -eq "ARM64") {
    $inputBinaries = "$icuSource\binARM64"
    $inputLibs     = "$icuSource\libARM64"
} else {
    throw "Error: Unsupported architecture."
}

# Use the same folder name for both to make the code signing setup simpler.
$outputBinariesLocation = "$output\bin"
$outputLibsLocation     = "$output\lib"

# Get the major ICU version number (ex: 64) to check ICU data DLL file.
$icuMajorVersion = $env:ICUVersion.split(".")[0]

# This is done as a sanity check in order to ensure that VS build didn't mess up, and that we actually
# have the real ICU data DLL (icudt*.dll) file and not the stubdata DLL (which doesn't contain any data).
if ((Get-Item "$inputBinaries\icudt$icuMajorVersion.dll").length -lt 5KB)
{
    Write-Host "The ICU data DLL appears to too small."
    throw "Error: The ICU data DLL file appears to be too small!"
}

# Note: We store the return value from New-Item in order to suppress the Cmdlet output to stdout.
Write-Host "Creating folder: $outputBinariesLocation"
$ret = New-Item -Path $outputBinariesLocation -ItemType "Directory"

Write-Host "Creating folder: $outputLibsLocation"
$ret = New-Item -Path $outputLibsLocation -ItemType "Directory"

Copy-Item "$inputBinaries\*.exe" -Destination $outputBinariesLocation -Recurse
Copy-Item "$inputBinaries\*.dll" -Destination $outputBinariesLocation -Recurse
Copy-Item "$inputLibs\*.lib" -Destination $outputLibsLocation -Recurse
Copy-Item "$inputLibs\*.pdb" -Destination $outputLibsLocation -Recurse
