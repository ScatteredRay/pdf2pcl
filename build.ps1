Param(
    [Parameter(Position=0)]
    [ValidateSet('configure', 'build', 'clean')]
    [string[]]$Task
)

$ErrorActionPreference = 'Stop';
$BuildConfig = "Debug" # Release has errors

function Invoke-ConfigureTask {
    mkdir build -ErrorAction SilentlyContinue
    cd build
    $VCToolchain = (Join-Path $PSScriptRoot "vcpkg\scripts\buildsystems\vcpkg.cmake")
    cmake -B . -S .. -DCMAKE_GENERATOR_PLATFORM=x64 -DCMAKE_BUILD_TYPE=$BuildConfig "-DCMAKE_TOOLCHAIN_FILE=$VCToolchain"
}

function Invoke-BuildTask {
    cd build
    cmake --build . --config $BuildConfig
}

function Invoke-CleanTask {
    rm -Recurse -Force build
}

function Invoke-Task {
    Param(
        [string]$Task
    )
    Push-Location
    cd $PSScriptRoot
    try {
        switch($Task) {
            'configure' {
                Invoke-ConfigureTask
            }
            'build' {
                Invoke-BuildTask
            }
            'clean' {
                Invoke-CleanTask
            }
        }
    }
    catch {
        Write-Error $_
    }
    Pop-Location
}

$Task | % { Invoke-Task -Task $_ }