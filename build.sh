#! /bin/bash

set -e

export TEMPLATE=template/template.html

OUT=_out

markup() {
    FILE="$1"
    local OUTPATH="${OUTPATH:-$FILE}"
    mkdir -p $(dirname "$OUT/$OUTPATH.html")
    echo "content/$FILE.mu" "->" "$OUT/$OUTPATH.html"
    ROOT="" CANONICAL="${CANONICAL:-/$FILE}" $MARKUP "content/$FILE.mu" >"$OUT/$OUTPATH.html"
}

cc markup/main.c -o markup/main -g
MARKUP=markup/main

cp template/style.css template/icon{.png,.svg} "$OUT/"

# Special pages
CANONICAL="/" markup index
markup 404

# Archive
markup gui

markup acre
markup orcs
markup memory
markup ld55
cp -r content/ld55 $OUT/ 

markup island-runes
cp -r content/island-runes $OUT/ 

markup git
cp -r content/git $OUT/
