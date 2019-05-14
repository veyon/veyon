#!/usr/bin/env bash

set -e

cd /veyon

flawfinder -Q -c core plugins master server service configurator cli worker

