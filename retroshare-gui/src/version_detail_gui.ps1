$scriptpath = $MyInvocation.MyCommand.Path
$dir = Split-Path $scriptpath
cd $dir

. ../../get_version.ps1

$htmlfile = ".\gui\help\version.html";
New-Item $htmlfile -type file -force;
"Retroshare Gui version : $version" | Out-File $htmlfile -Encoding ASCII -Append;
"Git Hash : $githash" | Out-File $htmlfile -Encoding ASCII -Append;
"Git info : $gitinfo" | Out-File $htmlfile -Encoding ASCII -Append;
"Git branch : $gitbranch" | Out-File $htmlfile -Encoding ASCII -Append;
("Build date: " + (Get-Date -Format g)) | Out-File $htmlfile -Encoding ASCII -Append;

