#! /bin/bash

set -e

export TEMPLATE=template/template.html

OUT=_out

markup() {
    OUTPATH="${1:-$OUTPATH}"
    mkdir -p $(dirname "$OUT/$OUTPATH.html")
    echo "content/$1.mu" "->" "$OUT/$OUTPATH.html"
    ROOT="" CANONICAL="${CANONICAL:-/$1}" $MARKUP "content/$1.mu" >"$OUT/$OUTPATH.html"
}

cc markup/main.c -o markup/main -g
MARKUP=markup/main

cp template/style.css template/icon{.png,.svg} "$OUT/"

# Special pages
CANONICAL="/" markup index
markup 404

# Archive
markup gui
