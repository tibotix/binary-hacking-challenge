#!/bin/sh

socat TCP-LISTEN:1024,reuseaddr,fork,su=chall EXEC:./cpue.py,stderr