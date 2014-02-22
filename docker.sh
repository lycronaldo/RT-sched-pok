#!/bin/sh

docker run --rm -ti -v $(pwd):/pok -w /pok -u root bob/pok_builder
