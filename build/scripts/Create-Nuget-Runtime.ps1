<#
.SYNOPSIS
    This script sets up the folder structure for the Windows Runtime Nuget package, copies/prepares the contents,
    and finally invokes the Nuget pack operation to create the package.

.PARAMETER sourceRoot
    The path to the root/top-level source code folder. Must already exist.

.PARAMETER icuBinaries
    The path to the location of the signed ICU binaries. Must already exist.
    Under this path there should be one folder per-arch: x64, x86, ARM, or ARM64.

.PARAMETER output
    The path to the output location. This script will create a subfolder named "nuget" for the Nuget package(s).
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
    [string]$output
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

$packageVersion = "$env:nugetPackageVersion-alpha$env:BUILD_BUILDNUMBER"
$packageName = 'Microsoft.ICU.icu4c'

$icuSource = Resolve-Path "$sourceRoot\icu\icu4c"

# Read in the various arch from the names of the folders (x64, x86, ARM, or ARM64).
$architectures = Get-ChildItem -Path $icuBinaries | Where-Object {$_.PSIsContainer} | ForEach-Object {$_.Name}

foreach ($arch in $architectures)
{
    # Create the staging folder for the nuget contents.
    $stagingLocation = "$output\nuget-win-$arch"

    # Note: We store the return value from New-Item in order to suppress the Cmdlet output to stdout.
    Write-Host 'Creating staging folder for the Nuget package...'
    $ret = New-Item -Path $stagingLocation -ItemType Directory

    # Create the folder structure for the Runtime Nuget
    Write-Host 'Creating folders'
    $ret = New-Item -Path "$stagingLocation\runtimes" -ItemType Directory
    $ret = New-Item -Path "$stagingLocation\runtimes\win-$arch" -ItemType Directory
    $ret = New-Item -Path "$stagingLocation\runtimes\win-$arch\native" -ItemType Directory
    
    # Compiled DLLs
    $dllInput = "$icuBinaries\$arch\bin\signed"
    $dllOutput = "$stagingLocation\runtimes\win-$arch\native"
    Copy-Item "$dllInput\*.dll" -Destination $dllOutput -Recurse

    # Add the License file
    Write-Host 'Copying the License file into the Nuget location.'
    $licenseFile = Resolve-Path "$icuSource\LICENSE"
    Copy-Item $licenseFile -Destination "$stagingLocation\LICENSE"

    # Create the version.txt file
    $versionTxtFile = "$stagingLocation\version.txt"
    $ret = New-Item -ItemType File -Force -Path $versionTxtFile
    "MS-ICU = $env:ICUVersion" | Set-Content -Encoding UTF8NoBOM $versionTxtFile
    "commit = $localSHA" | Add-Content -Encoding UTF8NoBOM $versionTxtFile

    # Update the placeholders in the template nuspec file.
    $runtimePackageId = "runtime.win-$arch.$packageName";
    $nuspecFileContent = (Get-Content "$sourceRoot\build\nuget\Template-runtime-arch.nuspec")
    $nuspecFileContent = $nuspecFileContent.replace('$runtimePackageId$', $runtimePackageId)
    $nuspecFileContent = $nuspecFileContent.replace('$version$', $packageVersion)
    $nuspecFileContent | Set-Content "$stagingLocation\$runtimePackageId.nuspec"

    # Actually do the "nuget pack" operation
    $nugetCmd = ("nuget pack $stagingLocation\$runtimePackageId.nuspec -BasePath $stagingLocation -OutputDirectory $output")
    Write-Host 'Executing: ' $nugetCmd
    &cmd /c $nugetCmd
}

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
"MS-ICU = $env:ICUVersion" | Set-Content -Encoding UTF8NoBOM $versionTxtFile
"commit = $localSHA" | Add-Content -Encoding UTF8NoBOM $versionTxtFile

# Create the runtime.json file
$runtimeFile = "$stagingLocation\runtime.json"
$ret = New-Item -ItemType File -Force -Path $runtimeFile

$rt = [pscustomobject]@{ "runtimes" = [pscustomobject]@{} }
foreach ($arch in $architectures)
{
    $runtimePackageId = "runtime.win-$arch.$packageName";
    $inner = [pscustomobject]@{ "$runtimePackageId" = "$packageVersion" }
    $outer = [pscustomobject]@{ "$packageName" = $inner }
    $rt.runtimes | add-member -membertype NoteProperty -name "win-$arch" -value $outer
}
$rt | ConvertTo-Json | Set-Content -Encoding UTF8NoBOM $runtimeFile

# Update the placeholders in the template nuspec file.
$runtimePackageId = "$packageName";
$nuspecFileContent = (Get-Content "$sourceRoot\build\nuget\Template-runtime.nuspec")
$nuspecFileContent = $nuspecFileContent.replace('$runtimePackageId$', $runtimePackageId)
$nuspecFileContent = $nuspecFileContent.replace('$version$', $packageVersion)
$nuspecFileContent | Set-Content "$stagingLocation\$runtimePackageId.nuspec"

# Actually do the "nuget pack" operation
$nugetCmd = ("nuget pack $stagingLocation\$runtimePackageId.nuspec -BasePath $stagingLocation -OutputDirectory $output")
Write-Host 'Executing: ' $nugetCmd
&cmd /c $nugetCmd
