Param(
    [Parameter(Position=0)]
    [ValidateSet('configure', 'build', 'clean')]
    [string[]]$Task
)

$ErrorActionPreference = 'Stop';

function Invoke-ConfigureTask {
    mkdir build -ErrorAction SilentlyContinue
    cd build
    cmake -B . -S .. -DCMAKE_GENERATOR_PLATFORM=x64 -DCMAKE_TOOLCHAIN_FILE=C:\Users\indy\dev\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_VERBOSE=ON -DVCPKG_TRACE_FIND_PACKAGE=ON -DCMAKE_FIND_DEBUG_MODE=1
}

function Invoke-BuildTask {
    cd build
    cmake --build .
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