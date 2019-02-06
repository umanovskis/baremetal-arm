#!/usr/bin/env bash

TEMPLATE=~/.pandoc/templates/eisvogel.tex

if [ ! -f "$TEMPLATE" ]; then
    TEMPLATE=~/pandoc-latex-template/eisvogel.tex
fi
if [ ! -f "$TEMPLATE" ]; then
    if [ $? -eq 1 ]; then
        TEMPLATE="$1"
    fi
fi
if [ ! -f "$TEMPLATE" ]; then
    echo "Cannot find eisvogel.tex anywhere. Install it, or point it out as an argument to this script"
    exit 1
fi

COMMIT=$(git rev-parse HEAD | cut -c -10)
pandoc *.md -o ebook.pdf --from markdown --template "$TEMPLATE" -V book  -V classoption=oneside -V header-right="$COMMIT" --listings --chapters
if [ -f ebook.pdf ] && [ -f upload.sh ]; then
    sh -xe upload.sh &
fi
