param(
    [string]$JsonPath = (Join-Path $PSScriptRoot 'slides_data.json'),
    [string]$OutFile  = (Join-Path $PSScriptRoot 'Onedow_게임발표.pptx')
)

$ErrorActionPreference = 'Stop'
$slides = Get-Content -LiteralPath $JsonPath -Raw -Encoding UTF8 | ConvertFrom-Json
$OutDir = Join-Path $PSScriptRoot '_pptx_build'
if (Test-Path $OutDir) { Remove-Item -LiteralPath $OutDir -Recurse -Force }
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null

function Escape-Xml([string]$s) {
    if ($null -eq $s) { return '' }
    return [System.Security.SecurityElement]::Escape($s)
}

function New-TextBoxXml([string]$text, [int]$x, [int]$y, [int]$cx, [int]$cy, [int]$fontSize, [bool]$bold) {
    if ($bold) {
        $rPr = '<a:rPr lang="ko-KR" sz="' + $fontSize + '" b="1" dirty="0"><a:solidFill><a:srgbClr val="E8EAED"/></a:solidFill><a:latin typeface="Malgun Gothic"/><a:ea typeface="Malgun Gothic"/></a:rPr>'
    } else {
        $rPr = '<a:rPr lang="ko-KR" sz="' + $fontSize + '" dirty="0"><a:solidFill><a:srgbClr val="C8CAD0"/></a:solidFill><a:latin typeface="Malgun Gothic"/><a:ea typeface="Malgun Gothic"/></a:rPr>'
    }
    $lines = $text -split "`n"
    $paras = New-Object System.Collections.Generic.List[string]
    foreach ($line in $lines) {
        $paras.Add('<a:p><a:r>' + $rPr + '<a:t>' + (Escape-Xml $line) + '</a:t></a:r></a:p>')
    }
    $paraRuns = $paras -join ''
    return @(
        '<p:sp>'
        '<p:nvSpPr><p:cNvPr id="2" name="TextBox"/><p:cNvSpPr txBox="1"/><p:nvPr/></p:nvSpPr>'
        '<p:spPr><a:xfrm><a:off x="' + $x + '" y="' + $y + '"/><a:ext cx="' + $cx + '" cy="' + $cy + '"/></a:xfrm>'
        '<a:prstGeom prst="rect"><a:avLst/></a:prstGeom><a:noFill/></p:spPr>'
        '<p:txBody><a:bodyPr wrap="square" rtlCol="0"/><a:lstStyle/>'
        $paraRuns
        '</p:txBody></p:sp>'
    ) -join ''
}

function New-SlideXml([string]$title, [string]$body, [int]$slideId) {
    $titleXml = New-TextBoxXml $title 457200 274638 8229600 914400 2800 $true
    $bodyXml  = New-TextBoxXml $body  457200 1600200 8229600 5029200 1600 $false
    return @(
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<p:sld xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"'
        ' xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"'
        ' xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">'
        '<p:cSld name="Slide ' + $slideId + '">'
        '<p:bg><p:bgPr><a:solidFill><a:srgbClr val="0A0A0C"/></a:solidFill><a:effectLst/></p:bgPr></p:bg>'
        '<p:spTree>'
        '<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>'
        '<p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr>'
        $titleXml
        $bodyXml
        '</p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>'
    ) -join ''
}

function Write-Utf8([string]$path, [string]$content) {
    $utf8 = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($path, $content, $utf8)
}

$dirs = @(
    '_rels', 'docProps', 'ppt', 'ppt\_rels', 'ppt\slides', 'ppt\slides\_rels',
    'ppt\slideLayouts', 'ppt\slideLayouts\_rels', 'ppt\slideMasters', 'ppt\slideMasters\_rels', 'ppt\theme'
)
foreach ($d in $dirs) { New-Item -ItemType Directory -Path (Join-Path $OutDir $d) -Force | Out-Null }

$slideCount = $slides.Count
for ($i = 0; $i -lt $slideCount; $i++) {
    $n = $i + 1
    Write-Utf8 (Join-Path $OutDir "ppt\slides\slide$n.xml") (New-SlideXml $slides[$i].title $slides[$i].body $n)
    Write-Utf8 (Join-Path $OutDir "ppt\slides\_rels\slide$n.xml.rels") @(
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>'
        '</Relationships>'
    ) -join ''
}

Write-Utf8 (Join-Path $OutDir 'ppt\theme\theme1.xml') @(
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="Onedow Dark">'
    '<a:themeElements><a:clrScheme name="Onedow">'
    '<a:dk1><a:srgbClr val="0A0A0C"/></a:dk1><a:lt1><a:srgbClr val="E8EAED"/></a:lt1>'
    '<a:dk2><a:srgbClr val="12141C"/></a:dk2><a:lt2><a:srgbClr val="4DF0FF"/></a:lt2>'
    '<a:accent1><a:srgbClr val="4DF0FF"/></a:accent1><a:accent2><a:srgbClr val="F23394"/></a:accent2>'
    '<a:accent3><a:srgbClr val="FF8C33"/></a:accent3><a:accent4><a:srgbClr val="E8384F"/></a:accent4>'
    '<a:accent5><a:srgbClr val="4A4E5A"/></a:accent5><a:accent6><a:srgbClr val="12141C"/></a:accent6>'
    '<a:hlink><a:srgbClr val="4DF0FF"/></a:hlink><a:folHlink><a:srgbClr val="F23394"/></a:folHlink>'
    '</a:clrScheme><a:fontScheme name="Office"><a:majorFont/><a:minorFont/></a:fontScheme>'
    '<a:fmtScheme name="Office"><a:fillStyleLst/><a:lnStyleLst/><a:effectStyleLst/><a:bgFillStyleLst/></a:fmtScheme>'
    '</a:themeElements></a:theme>'
) -join ''

Write-Utf8 (Join-Path $OutDir 'ppt\slideLayouts\slideLayout1.xml') @(
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<p:sldLayout xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" type="blank" preserve="1">'
    '<p:cSld name="Blank"><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr/></p:spTree></p:cSld>'
    '<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sldLayout>'
) -join ''

Write-Utf8 (Join-Path $OutDir 'ppt\slideLayouts\_rels\slideLayout1.xml.rels') @(
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
    '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="../slideMasters/slideMaster1.xml"/>'
    '</Relationships>'
) -join ''

Write-Utf8 (Join-Path $OutDir 'ppt\slideMasters\slideMaster1.xml') @(
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<p:sldMaster xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" preserve="1">'
    '<p:cSld><p:bg><p:bgRef idx="1001"><a:schemeClr val="bg1"/></p:bgRef></p:bg><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr/></p:spTree></p:cSld>'
    '<p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/>'
    '<p:sldLayoutIdLst><p:sldLayoutId id="2147483649" r:id="rId1"/></p:sldLayoutIdLst></p:sldMaster>'
) -join ''

Write-Utf8 (Join-Path $OutDir 'ppt\slideMasters\_rels\slideMaster1.xml.rels') @(
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
    '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>'
    '<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme" Target="../theme/theme1.xml"/>'
    '</Relationships>'
) -join ''

$sldIdLines = New-Object System.Collections.Generic.List[string]
$presRelLines = New-Object System.Collections.Generic.List[string]
for ($i = 1; $i -le $slideCount; $i++) {
    $id = 255 + $i
    $sldIdLines.Add('  <p:sldId id="' + $id + '" r:id="rId' + $i + '"/>')
    $presRelLines.Add('  <Relationship Id="rId' + $i + '" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide" Target="slides/slide' + $i + '.xml"/>')
}
$masterRid = $slideCount + 1
$themeRid  = $slideCount + 2

Write-Utf8 (Join-Path $OutDir 'ppt\presentation.xml') @(
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<p:presentation xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" saveSubsetFonts="1">'
    '<p:sldMasterIdLst><p:sldMasterId id="2147483648" r:id="rId' + $masterRid + '"/></p:sldMasterIdLst>'
    '<p:sldIdLst>'
    ($sldIdLines -join "`n")
    '</p:sldIdLst>'
    '<p:sldSz cx="9144000" cy="6858000" type="screen4x3"/>'
    '<p:notesSz cx="6858000" cy="9144000"/>'
    '</p:presentation>'
) -join "`n"

Write-Utf8 (Join-Path $OutDir 'ppt\_rels\presentation.xml.rels') @(
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
    ($presRelLines -join "`n")
    '  <Relationship Id="rId' + $masterRid + '" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="slideMasters/slideMaster1.xml"/>'
    '  <Relationship Id="rId' + $themeRid + '" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme" Target="theme/theme1.xml"/>'
    '</Relationships>'
) -join "`n"

Write-Utf8 (Join-Path $OutDir '_rels\.rels') @(
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
    '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="ppt/presentation.xml"/>'
    '<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" Target="docProps/core.xml"/>'
    '<Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>'
    '</Relationships>'
) -join ''

Write-Utf8 (Join-Path $OutDir 'docProps\core.xml') @(
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/">'
    '<dc:title>Onedow Game Presentation</dc:title>'
    '<dc:creator>Rid</dc:creator>'
    '</cp:coreProperties>'
) -join ''

Write-Utf8 (Join-Path $OutDir 'docProps\app.xml') @(
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties">'
    '<Application>WiNILL docs generator</Application>'
    '<Slides>' + $slideCount + '</Slides>'
    '</Properties>'
) -join ''

$overrides = New-Object System.Collections.Generic.List[string]
$overrides.Add('<Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/>')
$overrides.Add('<Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>')
$overrides.Add('<Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>')
$overrides.Add('<Override PartName="/ppt/slideMasters/slideMaster1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml"/>')
$overrides.Add('<Override PartName="/ppt/slideLayouts/slideLayout1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml"/>')
$overrides.Add('<Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/>')
for ($i = 1; $i -le $slideCount; $i++) {
    $overrides.Add('<Override PartName="/ppt/slides/slide' + $i + '.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/>')
}

Write-Utf8 (Join-Path $OutDir '[Content_Types].xml') @(
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
    '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
    '<Default Extension="xml" ContentType="application/xml"/>'
    ($overrides -join "`n")
    '</Types>'
) -join "`n"

if (Test-Path -LiteralPath $OutFile) { Remove-Item -LiteralPath $OutFile -Force }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($OutDir, $OutFile)
Remove-Item -LiteralPath $OutDir -Recurse -Force

Write-Output ('Created: ' + $OutFile + ' (' + $slideCount + ' slides)')
