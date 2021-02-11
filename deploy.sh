#! /bin/bash

set -e

cd "$(git rev-parse --show-toplevel)"

if ! git diff HEAD --exit-code --quiet; then
    echo "dirty! please commit changes first"
    exit 1
fi

./build.sh

COMMIT="$(git rev-parse HEAD)"

find _out/ -not -path '*.git/*' -type f -exec rm \{\} \;

cd _out/

git add -A

git commit -m "Deploy $COMMIT" || true
git push
