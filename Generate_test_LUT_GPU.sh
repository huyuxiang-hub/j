#!/bin/bash
#Author Name:Yuxiang Hu
#Creation date:2024-01-30
#Description: 

job_dir=/tmp/ihep/test_lut_profiling
log_dir=${job_dir}/log
mkdir -p ${log_dir}
mkdir -p $job_dir
pushd $job_dir
for ene in 1 10 100 1000
do
    for lut in 0 1 
    do
         
        oim=1
        run_job="run_${ene}MeV_OIM_${oim}_LUT_${lut}.sh"
        cat>"${run_job}"<<EOF
#!/bin/bash

source /home/ihep/j/jok.bash
export LUT=${lut}
export ENE=${ene}  #MeV
export OIM=${oim}   # only GPU
export GUN=4
export CATE="LUT_${lut}_ENE_${ene}_OIM_${oim}_GUN_4"
jok-tds
EOF
    chmod +x ${run_job}
    ./${run_job} >& ./log/${run_job}.log &
    done
done

popd
