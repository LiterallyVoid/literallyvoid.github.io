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

OUTPATH="acre" markup proglang/index
OUTPATH="acre/syntax" markup proglang/syntax
