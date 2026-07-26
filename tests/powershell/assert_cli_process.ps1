param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [Parameter(Mandatory = $true)]
    [ValidateSet("help", "invalid", "sequential", "mixed", "utf8")]
    [string]$Scenario,

    [Parameter(Mandatory = $true)]
    [string]$FixtureRoot
)

$OutputEncoding = [Console]::OutputEncoding = [System.Text.UTF8Encoding]::new()
$testRoot = $null
$expectedCsv = $null

switch ($Scenario) {
    "help" {
        $expectedExit = 0
        $expectedPattern = "Usage:"
        $cliArguments = @("--help")
    }
    "invalid" {
        $expectedExit = 64
        $expectedPattern = "[RESULT] status=failed code=64"
        $cliArguments = @("benchmark")
    }
    { $_ -in @("sequential", "mixed") } {
        $testRoot = Join-Path `
            ([System.IO.Path]::GetTempPath()) `
            ("parallelpix-m7-" + [guid]::NewGuid().ToString("N"))
        $inputDir = Join-Path $testRoot "输入"
        $outputDir = Join-Path $testRoot "输出"
        $resultDir = Join-Path $testRoot "结果"
        $watermark = Join-Path $testRoot "水印.png"
        $expectedCsv = Join-Path $resultDir "benchmark.csv"
        $null = New-Item -ItemType Directory -Path $inputDir -Force
        $null = New-Item -ItemType Directory -Path $resultDir -Force
        Copy-Item `
            -LiteralPath (Join-Path $FixtureRoot "images/color_3x2.png") `
            -Destination (Join-Path $inputDir "商品.png")
        Copy-Item `
            -LiteralPath (Join-Path $FixtureRoot "watermarks/rgba_2x2.png") `
            -Destination $watermark

        if ($Scenario -eq "sequential") {
            $expectedExit = 0
            $expectedPattern = "[RESULT] status=success code=0"
            $backends = "sequential"
        }
        else {
            $expectedExit = $null
            $expectedPattern = $null
            $backends = "sequential,openmp,cuda"
        }

        $cliArguments = @(
            "benchmark",
            "--input", $inputDir,
            "--output", $outputDir,
            "--watermark", $watermark,
            "--backends", $backends,
            "--image-counts", "1",
            "--threads", "2",
            "--cuda-batches", "1",
            "--warmups", "2",
            "--repetitions", "5",
            "--csv", $expectedCsv,
            "--append"
        )
    }
    "utf8" {
        $expectedExit = 64
        $unicodeArgument = [string]([char]0x6D4B) + [char]0x8BD5
        $expectedPattern = $unicodeArgument
        $cliArguments = @($unicodeArgument)
    }
}

$quotedArguments = $CliArguments | ForEach-Object {
    '"' + $_.Replace('"', '\"') + '"'
}
$strictUtf8 = [System.Text.UTF8Encoding]::new($false, $true)
$startInfo = [System.Diagnostics.ProcessStartInfo]::new()
$startInfo.FileName = $Executable
$startInfo.Arguments = $quotedArguments -join " "
$startInfo.UseShellExecute = $false
$startInfo.RedirectStandardOutput = $true
$startInfo.RedirectStandardError = $true
$startInfo.StandardOutputEncoding = $strictUtf8
$startInfo.StandardErrorEncoding = $strictUtf8

$process = [System.Diagnostics.Process]::new()
$process.StartInfo = $startInfo
$null = $process.Start()
$standardOutput = $process.StandardOutput.ReadToEnd()
$standardError = $process.StandardError.ReadToEnd()
$process.WaitForExit()

$combinedOutput = $standardOutput + $standardError
$actualExit = $process.ExitCode

Write-Output $combinedOutput

if ($Scenario -eq "mixed") {
    if ($actualExit -notin @(0, 2)) {
        Write-Error "Expected mixed exit code 0 or 2 but received $actualExit."
        exit 1
    }
}
elseif ($actualExit -ne $ExpectedExit) {
    Write-Error "Expected exit code $ExpectedExit but received $actualExit."
    exit 1
}

if ($null -ne $ExpectedPattern -and
    -not $combinedOutput.Contains($ExpectedPattern)) {
    Write-Error "Output did not contain expected text: $ExpectedPattern"
    exit 1
}

if ($Scenario -in @("sequential", "mixed")) {
    if (-not (Test-Path -LiteralPath $expectedCsv -PathType Leaf)) {
        Write-Error "Benchmark did not create CSV: $expectedCsv"
        exit 1
    }
    $rows = @(Import-Csv -LiteralPath $expectedCsv)
    if ($Scenario -eq "sequential") {
        if ($rows.Count -ne 1 -or $rows[0].backend -ne "sequential") {
            Write-Error "Benchmark CSV did not contain the expected Sequential row."
            exit 1
        }
        $expectedPngCount = 1
    }
    else {
        $backends = @($rows | ForEach-Object { $_.backend })
        if ($rows.Count -notin @(2, 3) -or
            $backends -notcontains "sequential" -or
            $backends -notcontains "openmp") {
            Write-Error "Mixed Benchmark CSV did not contain the required CPU rows."
            exit 1
        }
        $openmpRow = @($rows | Where-Object { $_.backend -eq "openmp" })[0]
        if ($openmpRow.validation_passed -ne "true" -or
            $openmpRow.thread_count -ne "2") {
            Write-Error "OpenMP row did not contain the expected validation and thread count."
            exit 1
        }

        $cudaRows = @($rows | Where-Object { $_.backend -eq "cuda" })
        if ($cudaRows.Count -eq 1) {
            $cudaRow = $cudaRows[0]
            if ($actualExit -ne 0 -or
                -not $combinedOutput.Contains("[RESULT] status=success code=0") -or
                $cudaRow.validation_passed -ne "true" -or
                [string]::IsNullOrWhiteSpace($cudaRow.h2d_ms) -or
                [string]::IsNullOrWhiteSpace($cudaRow.kernel_ms) -or
                [string]::IsNullOrWhiteSpace($cudaRow.d2h_ms)) {
                Write-Error "Available CUDA backend did not produce a valid timed row."
                exit 1
            }
            $expectedPngCount = 3
        }
        elseif ($cudaRows.Count -eq 0) {
            if ($actualExit -ne 2 -or
                -not $combinedOutput.Contains("[RESULT] status=partial code=2")) {
                Write-Error "Unavailable CUDA backend did not produce partial success."
                exit 1
            }
            $expectedPngCount = 2
        }
        else {
            Write-Error "Mixed Benchmark produced duplicate CUDA rows."
            exit 1
        }
    }
    $pngFiles = @(Get-ChildItem -LiteralPath $outputDir -Recurse -Filter *.png)
    if ($pngFiles.Count -ne $expectedPngCount) {
        Write-Error "Benchmark did not persist the expected number of PNG outputs."
        exit 1
    }
}

if ($null -ne $testRoot -and (Test-Path -LiteralPath $testRoot)) {
    Remove-Item -LiteralPath $testRoot -Recurse -Force
}

exit 0
