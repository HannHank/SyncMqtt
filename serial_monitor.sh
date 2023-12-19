#!/bin/bash
stty -F $1 raw 115200  && cat $1 
