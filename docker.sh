#!/bin/sh

docker run --rm -ti -v $(pwd):/pok -w /pok -u root richardchien/pok_builder
