$bytes = [System.IO.File]::ReadAllBytes('WiNILL\main.cpp')
Write-Output $bytes.Length
$enc949 = [System.Text.Encoding]::GetEncoding(949)
$sample = $bytes[0..3000]
$text = $enc949.GetString($sample)
$text | Out-File -Encoding utf8 _sample_cp949.txt
