#!/bin/sh

docker run --rm -ti -v $(pwd):/pok -w /pok -u $(id -u ${USER}):$(id -g ${USER}) bob/pok_builder
