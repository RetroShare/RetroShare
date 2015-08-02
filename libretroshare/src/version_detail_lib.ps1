$scriptpath = $MyInvocation.MyCommand.Path
$dir = Split-Path $scriptpath
cd $dir

. ../../get_version.ps1

$headerfile = ".\retroshare\rsautoversion.h";
New-Item $headerfile -type file -force;
"#define RS_REVISION_NUMBER $version" | Out-File $headerfile -Encoding ASCII -Append;
"#define RS_GIT_BRANCH `"$gitbranch`"" | Out-File $headerfile -Encoding ASCII -Append;
"#define RS_GIT_INFO `"$gitinfo`"" | Out-File $headerfile -Encoding ASCII -Append;
"#define RS_GIT_HASH `"$githash`"" | Out-File $headerfile -Encoding ASCII -Append;

