#!/bin/bash

#$ -V
#$ -j y

echo "***********************************************"
echo "LSF job ID: "$LSB_JOBID
echo "LSF task ID: "$LSB_JOBINDEX
echo "Run on host: "`hostname`
echo "Operating system: "`uname -s`
echo "Username: "`whoami`
echo "Started at: "`date`
echo "***********************************************"

ind=$((${LSB_JOBINDEX}-1))

start_ind=$(($ind * $batch_windows))
end_ind=$((($ind + 1) * $batch_windows - 1))

echo ${ind}
echo ${start_ind}
echo ${end_ind}

${PATH_TO_RELATE}/bin/Relate \
    --mode "BuildTopology" \
    --first_section $start_ind \
    --last_section $end_ind \
    --chunk_index ${chunk_index} \
    -o ${output} 2>> ${LSB_JOBINDEX}_build_c${chunk_index}.log 

echo "***********************************************"
echo "Finished at: "`date`
echo "***********************************************"
exit 0

