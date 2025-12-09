$extensions = @(".h", ".cpp", ".c", ".hpp", ".hxx", ".cxx")
$files = Get-ChildItem -Path . -Recurse -File | Where-Object { $extensions -contains $_.Extension }

$enc_gbk = [System.Text.Encoding]::GetEncoding(936)
$enc_utf8_bom = New-Object System.Text.UTF8Encoding $true

foreach ($file in $files) {
    try {
        $content = [System.IO.File]::ReadAllText($file.FullName, $enc_gbk)
        [System.IO.File]::WriteAllText($file.FullName, $content, $enc_utf8_bom)
        Write-Host "Converted: $($file.Name)" -ForegroundColor Green
    }
    catch {
        Write-Host "Error converting $($file.Name): $_" -ForegroundColor Red
    }
}