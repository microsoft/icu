<#
.SYNOPSIS
    This script sets up the folder structure for the Runtime Nuget package,
    it copies and prepares the contents, and finally invokes the Nuget pack
    operation in order to create the package.

.PARAMETER sourceRoot
    The path to the root/top-level source code folder. Must already exist.

.PARAMETER icuBinaries
    The path to the location of the signed ICU binaries. Must already exist.
    Under this path there should be one folder per runtime.
    For example: "win-x64", "win-x86", "win-arm64", etc.

.PARAMETER output
    The path to the output location. This script will create a subfolder
    named "nuget" for the Nuget package(s).
#>
param(
    [Parameter(Mandatory=$true)]
    [ValidateScript({Test-Path $_ -PathType Container})]
    [string]$sourceRoot,

    [Parameter(Mandatory=$true)]
    [ValidateScript({Test-Path $_ -PathType Container})]
    [string]$icuBinaries,

    [Parameter(Mandatory=$true)]
    [ValidateNotNullOrEmpty()]
    [string]$output,

    [Parameter(Mandatory=$false)]
    [switch]$prerelease
)

Function Get-GitLocalRevision {
    $ret = (& git.exe rev-parse HEAD 2>$null)
    If ($LASTEXITCODE -NE 0) {
        throw "Error: Failed to determine the local SHA1 from git."
    }
    return $ret
}

$localSHA = $env:BUILD_SOURCEVERSION

# If ADO didn't already set the commit hash, we'll ask git directly.
if (!($localSHA)) {
    $localSHA = (Get-GitLocalRevision)
    if (!($localSHA)) {
        throw "Error: Unable to determine the local git commit hash."
    }
}

# We should already have the ICU version set as a environment variable.
if (!(Test-Path 'env:ICUVersion')) {
    throw "Error: The ICU version environment variable is not set."
}
# We should already have the Nuget version set as a environment variable.
if (!(Test-Path 'env:nugetPackageVersion')) {
    throw "Error: The Nuget version environment variable is not set."
}

$packageName = 'Microsoft.ICU.ICU4C.Runtime'
$packageVersion = "$env:nugetPackageVersion"

if ($prerelease.IsPresent) {
    $packageVersion = "$packageVersion-alpha$env:BUILD_BUILDNUMBER"
}

$icuSource = Resolve-Path "$sourceRoot\icu\icu4c"

#------------------------------------------------
# Create each individual runtime package

# Read in the various runtime names from the names of the folders.
# Make sure the runtime name is all lowercase.
$runtimeIdentifiers = Get-ChildItem -Path $icuBinaries | Where-Object {$_.PSIsContainer} | ForEach-Object {$_.Name.ToLower()}

foreach ($rid in $runtimeIdentifiers)
{
    # Create the staging folder for the nuget contents.
    $stagingLocation = "$output\nuget-$rid"

    # Note: We store the return value from New-Item in order to suppress the Cmdlet output to stdout.
    Write-Host 'Creating staging folder for the Nuget package...'
    $ret = New-Item -Path $stagingLocation -ItemType Directory

    # Create the folder structure for the Runtime Nuget
    Write-Host 'Creating folders'
    $ret = New-Item -Path "$stagingLocation\runtimes" -ItemType Directory
    $ret = New-Item -Path "$stagingLocation\runtimes\$rid" -ItemType Directory
    $ret = New-Item -Path "$stagingLocation\runtimes\$rid\native" -ItemType Directory
    
    if ($rid.StartsWith('win'))
    {
        # Compiled DLLs
        $dllInput = "$icuBinaries\$rid\bin\signed"
        $dllOutput = "$stagingLocation\runtimes\$rid\native"
        Copy-Item "$dllInput\*.dll" -Destination $dllOutput -Recurse
    }

    # Add the License file
    Write-Host 'Copying the License file into the Nuget location.'
    $licenseFile = Resolve-Path "$icuSource\LICENSE"
    Copy-Item $licenseFile -Destination "$stagingLocation\LICENSE"

    # Create the version.txt file
    $versionTxtFile = "$stagingLocation\version.txt"
    $ret = New-Item -ItemType File -Force -Path $versionTxtFile
    "MS-ICU = $env:ICUVersion" | Set-Content -Encoding UTF8 $versionTxtFile
    "commit = $localSHA" | Add-Content -Encoding UTF8 $versionTxtFile

    # Update the placeholders in the template nuspec file.
    $runtimePackageId = "runtime.$rid.$packageName";
    $nuspecFileContent = (Get-Content "$sourceRoot\build\nuget\Template-runtime-rid.nuspec")
    $nuspecFileContent = $nuspecFileContent.replace('$runtimePackageId$', $runtimePackageId)
    $nuspecFileContent = $nuspecFileContent.replace('$version$', $packageVersion)
    $nuspecFileContent | Set-Content "$stagingLocation\$runtimePackageId.nuspec"

    # Actually do the "nuget pack" operation
    $nugetCmd = ("nuget pack $stagingLocation\$runtimePackageId.nuspec -BasePath $stagingLocation -OutputDirectory $output")
    Write-Host 'Executing: ' $nugetCmd
    &cmd /c $nugetCmd
}

#------------------------------------------------
# Create the meta-package

# Create the staging folder for the nuget contents.
$stagingLocation = "$output\nuget"

# Note: We store the return value from New-Item in order to suppress the Cmdlet output to stdout.
Write-Host 'Creating staging folder for the Nuget package...'
$ret = New-Item -Path $stagingLocation -ItemType Directory

# Add the License file
Write-Host 'Copying the License file into the Nuget location.'
$licenseFile = Resolve-Path "$icuSource\LICENSE"
Copy-Item $licenseFile -Destination "$stagingLocation\LICENSE"

# Create the version.txt file
$versionTxtFile = "$stagingLocation\version.txt"
$ret = New-Item -ItemType File -Force -Path $versionTxtFile
"MS-ICU = $env:ICUVersion" | Set-Content -Encoding UTF8 $versionTxtFile
"commit = $localSHA" | Add-Content -Encoding UTF8 $versionTxtFile

# Build up the list of runtime packages to add as dependencies to the meta package.
$deps = ""
foreach ($rid in $runtimeIdentifiers)
{
    $deps = $deps + "      <dependency id=`"runtime.$rid.$packageName`" version=`"$packageVersion`" />`r`n"
}

Write-Host "Adding these runtime packages:"
Write-Host $deps

# Update the placeholders in the template nuspec file.
$runtimePackageId = "$packageName";
$nuspecFileContent = (Get-Content "$sourceRoot\build\nuget\Template-runtime-meta.nuspec")
$nuspecFileContent = $nuspecFileContent.replace('$runtimePackageId$', $runtimePackageId)
$nuspecFileContent = $nuspecFileContent.replace('$version$', $packageVersion)
$nuspecFileContent = $nuspecFileContent.replace('$deps$', $deps)
$nuspecFileContent | Set-Content "$stagingLocation\$runtimePackageId.nuspec"

# Actually do the "nuget pack" operation
$nugetCmd = ("nuget pack $stagingLocation\$runtimePackageId.nuspec -BasePath $stagingLocation -OutputDirectory $output")
Write-Host 'Executing: ' $nugetCmd
&cmd /c $nugetCmd
