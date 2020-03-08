#!/bin/bash

function bump_source() {
  search='(let version = ").+(")'
  replace="\1$2\2"
  sed -i ".tmp" -E "s/${search}/${replace}/g" "$1"
  rm "$1.tmp"
}

function bump_brew() {
  search="(VERSION = ').+(')"
  replace="\1$2\2"
  sed -i ".tmp" -E "s/${search}/${replace}/g" "$1"
  rm "$1.tmp"
}

function bump_dune() {
  search="(\(version ).+(\))"
  replace="\1$2\2"
  sed -i ".tmp" -E "s/${search}/${replace}/g" "$1"
  rm "$1.tmp"
}

function bump_all() {
  output=$(npm version "${release}" --no-git-tag-version)
  version=${output:1}
  bump_source "bin/package.re" "$version"
  bump_brew "scripts/tmattio-repack.rb" "$version"
  bump_dune "dune-project" "$version"
}

function help() {
  echo "Usage: $(basename $0) [<newversion> | major | minor | patch | premajor | preminor | prepatch | prerelease]"
}

if [ -z "$1" ] || [ "$1" = "help" ]; then
  help
  exit
fi

release=$1

if [ -d ".git" ]; then
  changes=$(git status --porcelain)
  branch=$(git rev-parse --abbrev-ref HEAD)

  if [ -n "${changes}" ]; then
    echo "Please commit staged files prior to bumping"
    exit 1
  elif [ "${branch}" != "master" ]; then
    echo "Please run the release script on master"
    exit 1
  else
    bump_all
    git add .
    git commit -m "Bump to ${version}"
    git tag -a "${output}" -m "${version}"
    git push origin --tags
  fi
else
  bump_all
fi
