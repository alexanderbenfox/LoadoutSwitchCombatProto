# One-time setup: initializes git + LFS and makes the first commit.
# Run from this folder:  powershell -ExecutionPolicy Bypass -File .\setup-repo.ps1

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Error "git not found. Install Git for Windows (includes Git LFS): https://git-scm.com/download/win"
}
git lfs version | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Error "git-lfs not found. Install it: https://git-lfs.com"
}

if (Test-Path .git) {
    Write-Host "Repo already initialized, skipping git init."
} else {
    git init -b main
}

git lfs install --local
git config --local lfs.locksverify false   # set to true once a remote with LFS locking is added

git add .
git status --short
git commit -m "Initial project: UE 5.8 C++ module, GAS + Enhanced Input + Motion Warping, git LFS"

Write-Host ""
Write-Host "Done. LFS-tracked patterns:"
git lfs track
