#!/bin/bash
#Author Name:Yuxiang Hu
#Creation date:2024-01-30
#Description: 

source /home/ihep/j/jok.bash

export LUT=0
export ENE=100  #MeV
export OIM=1   # only GPU
export GUN=4
export CATE="LUT_${LUT}_ENE_${ENE}_OIM_${OIM}_GUN_${GUN}"
jok-tds

