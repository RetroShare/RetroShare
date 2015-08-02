#!/bin/bash

#note - though a single number is currently rather misleading
#a version numer for approx human reading is nice
version=$(($(git rev-list --count HEAD)+8613-2391))
#+8613-2391 to approx matchching SVN number on caves conversion

githash=$(git log --format="%H" -n 1)
gitinfo=$(git describe --tags)
gitbranch=$(git rev-parse --abbrev-ref HEAD)
