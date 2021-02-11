#! /bin/bash

set -e

export TEMPLATE=template/template.html

OUT=_out

markup() {
    mkdir -p $(dirname "$OUT/$1.html")
    ROOT="" CANONICAL="${CANONICAL:-/$1}" $MARKUP "content/$1.mu" >$OUT/$1.html
}

markup-index() {
    mkdir -p $(dirname "$OUT/$1.html")
    ROOT="" CANONICAL="${CANONICAL:-/$1}" $MARKUP "content/$1/index.mu" >$OUT/$1.html
}

cc markup/main.c -o markup/main -g
MARKUP=markup/main

cp template/style.css template/favicon{.png,.svg} "$OUT/"

markup 404
markup gui
CANONICAL="/" markup index
