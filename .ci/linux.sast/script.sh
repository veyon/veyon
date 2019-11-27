#!/usr/bin/env bash

set -e

cd $1

flawfinder -Q -c core plugins master server service configurator cli worker

