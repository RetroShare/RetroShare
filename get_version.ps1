$env:TERM="msys";

$version = ([int](git rev-list --count HEAD)+8613-2391)

$githash = git log --format="%H" -n 1
$gitinfo = git describe --tags
$gitbranch = git rev-parse --abbrev-ref HEAD
