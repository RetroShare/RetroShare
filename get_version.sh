#!/bin/bash

version=0x$(git log --pretty="%H" | head -1 | cut -c1-8)

githash=$(git log --format="%H" -n 1)
gitinfo=$(git describe --tags)
gitbranch=$(git rev-parse --abbrev-ref HEAD)

revcount=$(git rev-list --count HEAD)
