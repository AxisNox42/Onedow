# Resolves WiNILL.exe offsets from asteria_crash.log (offset=0x...) to function + line
# using build\windows\x64\Release\WiNILL.pdb.
# Usage: powershell -File scripts\resolve_crash_addr.ps1 A9F12 [62294 ...]
param([Parameter(Mandatory, ValueFromRemainingArguments)][string[]]$Offsets)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$rel  = Join-Path $root 'build\windows\x64\Release'
$work = Join-Path $root 'build\tests\crash-sym'
New-Item -ItemType Directory -Force $work | Out-Null

$src = @'
#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <cstdlib>
#pragma comment(lib,"dbghelp.lib")
int main(int c,char**v){
 HANDLE h=GetCurrentProcess(); SymSetOptions(SYMOPT_LOAD_LINES|SYMOPT_UNDNAME);
 SymInitialize(h,NULL,FALSE);
 DWORD64 base=0x10000000;
 SymLoadModuleEx(h,NULL,v[1],NULL,base,0,NULL,0);
 for(int i=2;i<c;i++){
  DWORD64 a=base+strtoull(v[i],0,16);
  char buf[sizeof(SYMBOL_INFO)+512]={0}; SYMBOL_INFO*s=(SYMBOL_INFO*)buf; s->SizeOfStruct=sizeof(SYMBOL_INFO); s->MaxNameLen=500;
  DWORD64 d=0; DWORD ld=0; IMAGEHLP_LINE64 ln={sizeof(ln)};
  printf("%s: ", v[i]);
  if(SymFromAddr(h,a,&d,s)) printf("%s+0x%llx ",s->Name,d);
  if(SymGetLineFromAddr64(h,a,&ld,&ln)) printf("%s:%lu",ln.FileName,ln.LineNumber);
  puts("");
 }
}
'@
$cpp = Join-Path $work 'sym.cpp'
$exe = Join-Path $work 'sym.exe'
if (-not (Test-Path $exe)) {
    Set-Content -Path $cpp -Value $src -Encoding ascii
    $installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
    $env:PATH = "$installer;$env:PATH"
    $vs = & (Join-Path $installer 'vswhere.exe') -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vs) { throw 'Visual Studio C++ tools required' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    & cmd.exe /d /s /c "call `"$vcvars`" >nul && cd /d `"$work`" && cl /nologo /EHsc sym.cpp /Fe:sym.exe >nul"
    if ($LASTEXITCODE -ne 0) { throw 'sym helper build failed' }
}
Push-Location $rel
try { & $exe 'WiNILL.exe' @Offsets } finally { Pop-Location }
